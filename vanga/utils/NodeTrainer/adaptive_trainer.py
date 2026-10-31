"""Node-level optimizer and adaptive Vanga topology search."""

from itertools import chain
import json
import math
import os
from pathlib import Path
import shutil
import time

import joblib
import numpy as np
from sklearn.preprocessing import StandardScaler
import torch
import torch.nn.functional as functional

from ...dataset import as_dataset, close_dataset, iter_batches
from .adaptive_export import evaluate_hard_vanga, export_adaptive_vanga, prune_vanga
from .adaptive_model import AdaptiveVangaModel
from .trainer import (array_batches, check_pass, choose_device, collect_initial, describe_input,
                      make_batch, resources, write_json)


class NodeTrainer:
  """Train one supplied Vanga NODE topology; never mutates its structure."""

  def __init__(self, *, epochs=10, patience=3, min_improvement=0.05,
               batch_size=1024, eval_batch_size=None, learning_rate=1e-3,
               input_mode='sparse', seed=0, log_every_batches=1000):
    if epochs <= 0 or patience <= 0 or batch_size <= 0:
      raise ValueError('epochs, patience and batch_size must be positive')
    if not 0 <= min_improvement < 1:
      raise ValueError('min_improvement must be in [0, 1)')
    if not math.isfinite(learning_rate) or learning_rate <= 0:
      raise ValueError('learning_rate must be finite and positive')
    if seed < 0 or log_every_batches < 0:
      raise ValueError('seed and log_every_batches must be nonnegative')
    self.epochs, self.patience = epochs, patience
    self.min_improvement, self.batch_size = min_improvement, batch_size
    self.eval_batch_size = eval_batch_size or batch_size
    self.learning_rate, self.input_mode = learning_rate, input_mode
    self.seed, self.log_every_batches = seed, log_every_batches

  @staticmethod
  def _soft_loss(model, dataset, scale, batch_size, n_features, device, input_mode):
    total, rows = 0.0, 0
    model.eval()
    with torch.no_grad():
      for features, labels in iter_batches(dataset, batch_size, n_features):
        inputs = make_batch(features, slice(None), device, input_mode, 1.0 / scale)
        targets = torch.as_tensor(labels, dtype=torch.float32, device=device)
        loss = functional.binary_cross_entropy_with_logits(
          model(inputs), targets, reduction='sum')
        total += float(loss.item())
        rows += len(labels)
    if not rows:
      raise ValueError('empty NODE validation Dataset')
    return total / rows

  def fit(self, model, primary, secondary, scale, device, *, initial_batch=None,
          initial_exhausted=False,
          n_features=None, on_epoch=None):
    """Warm-start model parameters, reset Adam, train, and restore best secondary loss."""
    n_features = n_features or model.n_features
    torch.manual_seed(self.seed)
    if device.type == 'cuda':
      torch.cuda.manual_seed_all(self.seed)
    optimizer = torch.optim.Adam(model.parameters(), lr=self.learning_rate)
    rng = np.random.default_rng(self.seed)
    best_loss, best_epoch, best_state = float('inf'), 0, None
    significant_best, stale = float('inf'), 0
    history = []

    for epoch in range(1, self.epochs + 1):
      model.train()
      if epoch == 1 and initial_batch is not None:
        source_batches = (() if initial_exhausted else
                          iter_batches(primary, self.batch_size, n_features))
        batches = chain(array_batches(*initial_batch, self.batch_size), source_batches)
      else:
        batches = iter_batches(primary, self.batch_size, n_features)
      seen, train_loss = 0, 0.0
      for batch_number, (features, labels) in enumerate(batches, 1):
        order = rng.permutation(len(labels))
        inputs = make_batch(features, order, device, self.input_mode, 1.0 / scale)
        targets = torch.as_tensor(labels[order], dtype=torch.float32, device=device)
        optimizer.zero_grad(set_to_none=True)
        loss = functional.binary_cross_entropy_with_logits(model(inputs), targets)
        if not torch.isfinite(loss):
          raise ValueError('non-finite NODE training loss')
        loss.backward()
        optimizer.step()
        train_loss += float(loss.item()) * len(labels)
        seen += len(labels)
        if self.log_every_batches and batch_number % self.log_every_batches == 0 and on_epoch:
          on_epoch({'event': 'batch', 'epoch': epoch, 'rows': seen,
                    'train_logloss': train_loss / seen})
      if not seen:
        raise ValueError('empty primary training Dataset')

      secondary_loss = self._soft_loss(model, secondary, scale, self.eval_batch_size,
                                       n_features, device, self.input_mode)
      train_epoch_loss = train_loss / seen
      if secondary_loss < best_loss:
        best_loss, best_epoch = secondary_loss, epoch
        best_state = {key: value.detach().cpu().clone()
                      for key, value in model.state_dict().items()}
      if secondary_loss < significant_best * (1.0 - self.min_improvement):
        significant_best, stale = secondary_loss, 0
      else:
        stale += 1
      entry = {'epoch': epoch, 'train_logloss': train_epoch_loss,
               'secondary_soft_logloss': secondary_loss, 'best_epoch': best_epoch,
               'stale_epochs': stale}
      history.append(entry)
      if on_epoch:
        on_epoch(entry)
      if stale >= self.patience:
        break

    if best_state is None:
      raise RuntimeError('NODE training did not produce a checkpoint')
    model.load_state_dict(best_state)
    return {'best_secondary_soft_logloss': best_loss, 'best_epoch': best_epoch,
            'epochs_completed': len(history), 'history': history}


class VangaTrainer:
  """Adaptive one-tree Vanga topology search using NodeTrainer for each step."""

  def __init__(self, *, n_features=16384, mutation_nodes=15, extend_extra_nodes=5,
               epochs=10, node_patience=3, topology_patience=5, min_improvement=0.05,
               batch_size=1024, eval_batch_size=None, init_rows=1000,
               learning_rate=1e-3, seed=0, cpu_threads=4, device='cuda',
               input_mode='sparse', log_every_batches=1000, verbose=True):
    if min(mutation_nodes, extend_extra_nodes, n_features, epochs, node_patience,
           topology_patience, batch_size, init_rows, cpu_threads) <= 0:
      raise ValueError('node counts, epochs, patience, batch size and n_features must be positive')
    if not 0 <= min_improvement < 1:
      raise ValueError('min_improvement must be in [0, 1)')
    if input_mode not in ('sparse', 'dense'):
      raise ValueError('input_mode must be sparse or dense')
    self.settings = dict(
      n_features=n_features, mutation_nodes=mutation_nodes,
      extend_extra_nodes=extend_extra_nodes, epochs=epochs, node_patience=node_patience,
      topology_patience=topology_patience, min_improvement=min_improvement,
      batch_size=batch_size, eval_batch_size=eval_batch_size or batch_size,
      init_rows=init_rows, learning_rate=learning_rate, seed=seed,
      cpu_threads=cpu_threads, device=device, input_mode=input_mode,
      log_every_batches=log_every_batches, verbose=verbose)
    self.model_ = self.scaler_ = self.metrics_ = self.output_dir_ = None

  def _report(self, record):
    if self.settings['verbose']:
      print(json.dumps(record), flush=True)

  def fit(self, primary_train, secondary_train, validation, output_dir):
    """Train from an empty root; secondary data drives NODE stopping and pruning."""
    args = self.settings
    sources = []
    output = Path(output_dir).resolve()
    if output.exists():
      raise ValueError('output directory already exists: ' + str(output))
    output.mkdir(parents=True)
    (output / 'steps').mkdir()
    device = choose_device(args['device'])
    torch.set_num_threads(args['cpu_threads'])
    torch.manual_seed(args['seed'])
    np.random.seed(args['seed'])
    if device.type == 'cuda':
      torch.cuda.manual_seed_all(args['seed'])
    self.output_dir_ = output
    started = time.monotonic()
    result = {'status': 'loading', 'algorithm': 'adaptive-vanga',
              'settings': dict(args, primary_train=describe_input(primary_train),
                               secondary_train=describe_input(secondary_train),
                               validation=describe_input(validation), output_dir=str(output)),
              'device': str(device), 'history': []}
    self.metrics_ = result
    write_json(output / 'metrics.json', result)
    try:
      for data in (primary_train, secondary_train, validation):
        sources.append(as_dataset(data, args['n_features']))
      primary, secondary, val = (item[0] for item in sources)
      initial_x, initial_y, initial_exhausted = collect_initial(
        primary, args['init_rows'], args['batch_size'], args['n_features'])
      scaler = StandardScaler(with_mean=False).fit(initial_x)
      scaler.scale_ = np.asarray(scaler.scale_, dtype=np.float32)
      if not np.isfinite(scaler.scale_).all() or (scaler.scale_ <= 0).any():
        raise ValueError('invalid feature scales')
      scale = scaler.scale_
      inverse_scale = 1.0 / scale
      if not np.isfinite(inverse_scale).all():
        raise ValueError('invalid inverse feature scales')
      joblib.dump(scaler, output / 'feature_scaler.joblib')

      model = AdaptiveVangaModel(args['n_features'], device=device)
      prior = (float(np.sum(initial_y)) + 0.5) / (len(initial_y) + 1)
      with torch.no_grad():
        model.root.value.fill_(math.log(prior / (1.0 - prior)))
      initial_x_scaled = initial_x.copy().tocsr()
      initial_x_scaled.data *= inverse_scale[initial_x_scaled.indices]
      initial_batch = (initial_x, initial_y)
      rng = np.random.default_rng(args['seed'])
      optimizer = NodeTrainer(epochs=args['epochs'], patience=args['node_patience'],
                              min_improvement=args['min_improvement'],
                              batch_size=args['batch_size'],
                              eval_batch_size=args['eval_batch_size'],
                              learning_rate=args['learning_rate'], input_mode=args['input_mode'],
                              seed=args['seed'], log_every_batches=args['log_every_batches'])
      best_loss, best_step, stale_steps = float('inf'), None, 0
      previous_mode, extend_loss = None, None
      mode = 'extend'
      result.update(status='training', rows={'primary_train': None,
                                             'secondary_train': None, 'validation': None})
      write_json(output / 'metrics.json', result)

      step = 0
      while True:
        step += 1
        add_count = args['mutation_nodes'] + (
          args['extend_extra_nodes'] if mode == 'extend' else 0)
        added = model.add_random_splits(add_count, rng, device=device)
        model.initialize_added_splits(added, initial_x_scaled, rng)
        first_batch = initial_batch if step == 1 else None
        node_result = optimizer.fit(model, primary, secondary, scale, device,
                                    initial_batch=first_batch,
                                    initial_exhausted=initial_exhausted,
                                    on_epoch=lambda item: self._report(
                                      {'step': step, 'mode': mode, **item}))
        step_dir = output / 'steps' / ('step-{:04d}'.format(step))
        step_dir.mkdir()
        prune = prune_vanga(model, secondary, scale, args['mutation_nodes'],
                            args['eval_batch_size'], args['n_features'])
        exported = export_adaptive_vanga(model, scale, step_dir)
        val_metrics = evaluate_hard_vanga(model, val, scale, args['eval_batch_size'],
                                          args['n_features'])
        secondary_hard = evaluate_hard_vanga(
          model, secondary, scale, args['eval_batch_size'], args['n_features'])
        topology = model.topology()
        checkpoint = {'format_version': 1, 'topology': topology,
                      'state_dict': {key: value.detach().cpu() for key, value
                                     in model.state_dict().items()},
                      'n_features': args['n_features'], 'scale': scale.tolist(),
                      'step': step, 'mode': mode}
        torch.save(checkpoint, step_dir / 'node_model.pt')
        checkpoint_metadata = {
          'step': step, 'mode': mode, 'added_vanga_nodes': add_count,
          'pruning': prune, 'node_training': node_result,
          'validation_logloss': val_metrics['logloss'],
          'secondary_vanga_logloss': secondary_hard['logloss'],
          'vanga_nodes': model.branch_count(), 'container_nodes': exported['container_nodes'],
          'model_file_bytes': Path(exported['native_model']).stat().st_size,
          'node_checkpoint': str(step_dir / 'node_model.pt'),
          'vanga_model': exported['native_model']}
        write_json(step_dir / 'metrics.json', checkpoint_metadata)
        result['history'].append(checkpoint_metadata)
        score = val_metrics['logloss']
        improved_best = score < best_loss * (1.0 - args['min_improvement'])
        if score < best_loss:
          best_loss, best_step = score, step
          result['best_step'] = step
          result['best_validation_logloss'] = score
          shutil.copy2(step_dir / 'node_model.pt', output / 'best_node_model.pt')
          shutil.copy2(exported['native_model'], output / 'best_vanga_model.vanga')
        if improved_best:
          stale_steps = 0
        else:
          stale_steps += 1
        result.update(status='training', best_step=best_step,
                      best_validation_logloss=best_loss,
                      topology_steps_without_significant_improvement=stale_steps,
                      current_vanga_nodes=model.branch_count(),
                      elapsed_seconds=time.monotonic() - started)
        write_json(output / 'metrics.json', result)
        self._report({'event': 'step_finished', **checkpoint_metadata,
                      'best_validation_logloss': best_loss,
                      'stale_steps': stale_steps})

        if mode == 'extend':
          extend_loss = score
          next_mode = 'inplace'
        elif previous_mode == 'extend':
          next_mode = ('inplace' if score < extend_loss * (1.0 - args['min_improvement'])
                       else 'extend')
        else:
          next_mode = ('inplace' if improved_best else 'extend')
        previous_mode, mode = mode, next_mode
        if stale_steps >= args['topology_patience']:
          break

      result.update(status='complete', best_step=best_step,
                    best_validation_logloss=best_loss,
                    elapsed_seconds=time.monotonic() - started,
                    resources=resources(device))
      write_json(output / 'metrics.json', result)
      best_checkpoint = torch.load(output / 'best_node_model.pt', map_location=device,
                                   weights_only=True)
      best_model = AdaptiveVangaModel.from_topology(
        args['n_features'], best_checkpoint['topology'], device=device)
      best_model.load_state_dict(best_checkpoint['state_dict'])
      self.model_, self.scaler_ = best_model, scaler
      self.metrics_ = result
      return self
    except BaseException as error:
      result.update(status='failed', error_type=type(error).__name__, error=str(error),
                    elapsed_seconds=time.monotonic() - started)
      write_json(output / 'metrics.json', result)
      raise
    finally:
      for source, owned in sources:
        if owned:
          close_dataset(source)

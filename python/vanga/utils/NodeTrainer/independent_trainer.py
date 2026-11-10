"""Independent adaptive Vanga runs, function-preserving merge and final fit."""

import json
from pathlib import Path
import time

import joblib
import numpy as np
import torch

from ...dataset import as_dataset, close_dataset
from .adaptive_export import evaluate_hard_vanga, export_adaptive_vanga, prune_vanga
from .adaptive_model import AdaptiveVangaModel, VangaSplit
from .adaptive_trainer import NodeTrainer, VangaTrainer
from .trainer import choose_device, describe_input, resources, write_json


def _clone_split(source, target_container, n_features, device, usage_scale):
  target = VangaSplit(n_features, device=device)
  target_container.splits.append(target)
  with torch.no_grad():
    target.feature_logits.copy_(source.feature_logits)
    target.threshold.copy_(source.threshold)
    target.log_temperature.copy_(source.log_temperature)
    target.usage.copy_(source.usage * usage_scale)
  _copy_container(source.yes, target.yes, n_features, device)
  _copy_container(source.no, target.no, n_features, device)


def _copy_container(source, target, n_features, device):
  with torch.no_grad():
    target.value.copy_(source.value)
  for split in source.splits:
    _clone_split(split, target, n_features, device, 1.0)


def merge_vanga_models(models, n_features, device):
  """Average model functions by placing their root splits in one Vanga root."""
  if not models:
    raise ValueError('at least one model is required')
  merged = AdaptiveVangaModel(n_features, device=device)
  scale = 1.0 / len(models)
  with torch.no_grad():
    merged.root.value.copy_(torch.stack([model.root.value.detach().to(device)
                                         for model in models]).mean())
  for model in models:
    for split in model.root.splits:
      _clone_split(split, merged.root, n_features, device, scale)
  return merged


class IndependentVangaTrainer:
  """Run adaptive Vanga independently, merge the best trees, then fit and prune."""

  def __init__(self, *, independent_tries=1, n_features=16384,
               mutation_nodes=15, extend_extra_nodes=5, prune_epsilon=1e-6,
               epochs=10, node_patience=3, topology_patience=5,
               min_improvement=0.05, batch_size=1024, eval_batch_size=None,
               init_rows=1000, learning_rate=1e-3, seed=0, cpu_threads=4,
               device='cuda', input_mode='sparse', log_every_batches=1000,
               verbose=True):
    if independent_tries <= 0:
      raise ValueError('independent_tries must be positive')
    if seed < 0:
      raise ValueError('seed must be nonnegative')
    self.independent_tries = independent_tries
    self.settings = dict(
      independent_tries=independent_tries, n_features=n_features,
      mutation_nodes=mutation_nodes, extend_extra_nodes=extend_extra_nodes,
      prune_epsilon=prune_epsilon, epochs=epochs, node_patience=node_patience,
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
    args = self.settings
    output = Path(output_dir).resolve()
    if output.exists():
      raise ValueError('output directory already exists: ' + str(output))
    output.mkdir(parents=True)
    tries_dir = output / 'tries'
    tries_dir.mkdir()
    started = time.monotonic()
    result = {
      'status': 'running', 'algorithm': 'independent-adaptive-vanga',
      'settings': dict(args, primary_train=describe_input(primary_train),
                       secondary_train=describe_input(secondary_train),
                       validation=describe_input(validation), output_dir=str(output)),
      'tries': [], 'device': args['device']}
    self.output_dir_, self.metrics_ = output, result
    write_json(output / 'metrics.json', result)
    device = None
    models = []
    scaler = None
    sources = []
    try:
      device = choose_device(args['device'])
      torch.set_num_threads(args['cpu_threads'])
      for index in range(args['independent_tries']):
        try_seed = args['seed'] + index
        try_dir = tries_dir / ('try-{:04d}'.format(index + 1))
        self._report({'event': 'independent_try_started', 'try': index + 1,
                      'seed': try_seed})
        trainer = VangaTrainer(
          n_features=args['n_features'], mutation_nodes=args['mutation_nodes'],
          extend_extra_nodes=args['extend_extra_nodes'], prune_epsilon=args['prune_epsilon'],
          epochs=args['epochs'], node_patience=args['node_patience'],
          topology_patience=args['topology_patience'], min_improvement=args['min_improvement'],
          batch_size=args['batch_size'], eval_batch_size=args['eval_batch_size'],
          init_rows=args['init_rows'], learning_rate=args['learning_rate'],
          seed=try_seed, cpu_threads=args['cpu_threads'], device=args['device'],
          input_mode=args['input_mode'], log_every_batches=args['log_every_batches'],
          verbose=args['verbose'])
        trainer.fit(primary_train, secondary_train, validation, try_dir)
        if scaler is None:
          scaler = trainer.scaler_
        elif not np.array_equal(scaler.scale_, trainer.scaler_.scale_):
          raise ValueError('independent tries produced different feature scales')
        model = trainer.model_.to('cpu')
        models.append(model)
        summary = {
          'try': index + 1, 'seed': try_seed,
          'best_step': trainer.metrics_['best_step'],
          'best_validation_logloss': trainer.metrics_['best_validation_logloss'],
          'vanga_nodes': model.branch_count(), 'output_dir': str(try_dir)}
        result['tries'].append(summary)
        result.update(completed_tries=len(models), elapsed_seconds=time.monotonic() - started)
        write_json(output / 'metrics.json', result)
        self._report({'event': 'independent_try_finished', **summary})
        del trainer
        if device.type == 'cuda':
          torch.cuda.empty_cache()

      try_count = len(models)
      merged = merge_vanga_models(models, args['n_features'], device)
      models.clear()
      input_nodes = merged.branch_count()
      result.update(status='merged-node-training', merged_vanga_nodes=input_nodes,
                    elapsed_seconds=time.monotonic() - started)
      write_json(output / 'metrics.json', result)
      self._report({'event': 'merge_finished', 'trees': len(models),
                    'merged_vanga_nodes': input_nodes})

      for dataset in (primary_train, secondary_train, validation):
        sources.append(as_dataset(dataset, args['n_features']))
      primary, secondary, val = (item[0] for item in sources)
      joblib.dump(scaler, output / 'feature_scaler.joblib')
      final_node_seed = args['seed'] + args['independent_tries']
      final_optimizer = NodeTrainer(
        epochs=args['epochs'], patience=args['node_patience'],
        min_improvement=args['min_improvement'], batch_size=args['batch_size'],
        eval_batch_size=args['eval_batch_size'], learning_rate=args['learning_rate'],
        input_mode=args['input_mode'], seed=final_node_seed,
        log_every_batches=args['log_every_batches'])
      final_training = final_optimizer.fit(
        merged, primary, secondary, scaler.scale_, device,
        on_epoch=lambda item: self._report({'phase': 'merged-node', **item}))

      pruning = prune_vanga(merged, secondary, scaler.scale_, input_nodes,
                            args['eval_batch_size'], args['n_features'],
                            epsilon=args['prune_epsilon'])
      exported = export_adaptive_vanga(
        merged, scaler.scale_, output, filename='best_vanga_model.vanga')
      validation_metrics = evaluate_hard_vanga(
        merged, val, scaler.scale_, args['eval_batch_size'], args['n_features'])
      secondary_metrics = evaluate_hard_vanga(
        merged, secondary, scaler.scale_, args['eval_batch_size'], args['n_features'])
      checkpoint = {
        'format_version': 1, 'topology': merged.topology(),
        'state_dict': {key: value.detach().cpu() for key, value
                       in merged.state_dict().items()},
        'n_features': args['n_features'], 'scale': np.asarray(scaler.scale_).tolist(),
        'merged_tries': args['independent_tries'],
        'final_node_seed': final_node_seed}
      torch.save(checkpoint, output / 'best_node_model.pt')
      result.update(
        status='complete', merge={'input_vanga_nodes': input_nodes,
                                  'final_vanga_nodes': merged.branch_count(),
                                  'trees': try_count},
        final_node_training=final_training, pruning=pruning,
        secondary_logloss=secondary_metrics['logloss'],
        validation_logloss=validation_metrics['logloss'],
        best_validation_logloss=validation_metrics['logloss'],
        native_model=exported['native_model'],
        merged_checkpoint=str(output / 'best_node_model.pt'),
        elapsed_seconds=time.monotonic() - started,
        resources=resources(device))
      write_json(output / 'metrics.json', result)
      self.model_, self.scaler_, self.metrics_ = merged, scaler, result
      self._report({'event': 'independent_training_complete',
                    'validation_logloss': validation_metrics['logloss'],
                    'vanga_nodes': merged.branch_count(),
                    'pruning': pruning})
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

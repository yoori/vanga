"""Reusable sparse tree trainer, adapted from catboost-nn-train/train_node.py.

  from vanga import LibSvmFileDataset, VangaTrainer

  trainer = VangaTrainer(num_trees=100, max_depth=6, features_per_node=2, device='cuda')
  train = LibSvmFileDataset('train.libsvm.gz', n_features=16384)
  validation = LibSvmFileDataset('validation.libsvm.gz', n_features=16384)
  trainer.fit(train, validation, output_dir='run')
  probabilities = trainer.predict_proba('validation.libsvm.gz')
  restored = VangaTrainer.load('run', device='cpu')

fit consumes Dataset.get_rows(batch_size) incrementally, without a full cache.
After returning None a Dataset must restart on the next get_rows() call. Scaler
and model initialization use only the first init_rows training records. fit
also accepts file paths and (X, y) pairs as compatibility adapters. Matrix
column 0 corresponds to one-based LibSVM feature ID 1. predict methods consume
raw features and use the soft gates of the best checkpoint. fit additionally
exports hardened trees and reports their logloss separately in metrics_.
"""

import gzip
import hashlib
import importlib.metadata
from itertools import chain
import json
import math
from os import PathLike
import resource
import time
from pathlib import Path
from types import SimpleNamespace

import joblib
import numpy as np
from scipy import sparse
from sklearn.datasets import load_svmlight_file
from sklearn.preprocessing import StandardScaler
import torch
import torch.nn.functional as functional
from pytorch_tabular.models.common.layers import ODST, activations

from ...dataset import (Dataset, LibSvmFileDataset, as_dataset, close_dataset, iter_batches,
                        normalize_features, normalize_targets, validate_count, validate_rows)
from .config import validate_settings
from .export import export_catboost
from .multibranch import initialize_vanga, make_vanga_model, vanga_logits
from .vanga_export import export_vanga


def write_json(path, value):
  partial = path.with_name(path.name + '.partial')
  partial.write_text(json.dumps(value, indent=2, allow_nan=False) + '\n')
  partial.replace(path)


def sha256(path):
  digest = hashlib.sha256()
  with path.open('rb') as source:
    for block in iter(lambda: source.read(4 * 1024 * 1024), b''):
      digest.update(block)
  return digest.hexdigest()


def load_libsvm(path, n_features):
  before = path.stat()
  digest = sha256(path)
  opener = gzip.open if path.suffix == '.gz' else open
  with opener(path, 'rb') as source:
    features, labels = load_svmlight_file(
      source, n_features=n_features, zero_based=False, dtype=np.float32)
  after = path.stat()
  if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
    raise ValueError('input changed while loading: ' + str(path))
  if not len(labels):
    raise ValueError('empty dataset: ' + str(path))
  labels = normalize_targets(labels, len(labels))
  if not np.isfinite(features.data).all():
    raise ValueError('non-finite feature values: ' + str(path))
  features = sparse.csr_matrix(features, dtype=np.float32)
  features.sum_duplicates()
  features.eliminate_zeros()
  return features, labels, digest


def load_training_data(data, n_features):
  if isinstance(data, (str, PathLike)):
    return load_libsvm(Path(data), n_features)
  if not isinstance(data, tuple) or len(data) != 2:
    raise ValueError('training data must be a LibSVM path or an (X, y) pair')
  features = normalize_features(data[0], n_features)
  return features, normalize_targets(data[1], features.shape[0]), None


def describe_input(data):
  if isinstance(data, LibSvmFileDataset):
    return str(data.path)
  if isinstance(data, Dataset):
    return {'type': type(data).__module__ + '.' + type(data).__qualname__}
  if isinstance(data, (str, PathLike)):
    return str(Path(data).resolve())
  return {'type': 'memory', 'rows': int(data[0].shape[0]), 'columns': int(data[0].shape[1])}


def choose_device(name):
  device = torch.device(name)
  if device.type not in ('cpu', 'cuda'):
    raise ValueError('--device must be cpu, cuda or cuda:INDEX')
  if device.type == 'cuda':
    if not torch.cuda.is_available():
      raise RuntimeError('CUDA requested but unavailable; use --device cpu for a CPU test')
    index = device.index if device.index is not None else torch.cuda.current_device()
    if index >= torch.cuda.device_count():
      raise ValueError('CUDA device index does not exist: ' + str(index))
    device = torch.device('cuda', index)
    torch.cuda.set_device(device)
    torch.cuda.reset_peak_memory_stats(device)
  return device


def single_feature_selectors(values, dim=0):
  soft = activations.entmax15(values, dim=dim)
  hard = torch.zeros_like(soft).scatter_(dim, values.argmax(dim=dim, keepdim=True), 1)
  # Exact one-hot forward values; entmax supplies the surrogate selection gradient.
  return hard + (soft - soft.detach())


def make_model(n_features, num_trees, depth, feature_mode='projection', features_per_node=2):
  if feature_mode == 'vanga':
    return make_vanga_model(n_features, num_trees, depth, features_per_node)
  if feature_mode not in ('projection', 'single'):
    raise ValueError('unsupported feature mode: ' + str(feature_mode))
  choice = single_feature_selectors if feature_mode == 'single' else activations.entmax15
  model = ODST(in_features=n_features, num_trees=num_trees, depth=depth,
               tree_output_dim=2, flatten_output=False,
               choice_function=choice, bin_function=activations.entmoid15)
  model.feature_mode = feature_mode
  return model


def make_batch(features, indices, device, mode, inverse_scale=None):
  batch = features[indices].tocsr(copy=False)
  if inverse_scale is not None:
    batch.data *= inverse_scale[batch.indices]
    if not np.isfinite(batch.data).all():
      raise ValueError('non-finite scaled feature values')
  if mode == 'dense':
    return torch.as_tensor(batch.toarray(), dtype=torch.float32, device=device)
  rows = torch.as_tensor(batch.indptr.astype(np.int64), device=device)
  columns = torch.as_tensor(batch.indices.astype(np.int64), device=device)
  values = torch.as_tensor(batch.data, dtype=torch.float32, device=device)
  return torch.sparse_csr_tensor(rows, columns, values, size=batch.shape, device=device)


def logits(model, inputs):
  if model.feature_mode == 'vanga':
    return vanga_logits(model, inputs)
  if inputs.layout == torch.strided:
    return model(inputs).mean(dim=1)
  # Only selector projections and tree activations are dense, never the full input matrix.
  selectors = model.choice_function(model.feature_selection_logits, dim=0)
  values = torch.sparse.mm(inputs, selectors.reshape(inputs.shape[1], -1).contiguous())
  values = values.reshape(inputs.shape[0], model.num_trees, model.depth)
  thresholds = (values - model.feature_thresholds) * torch.exp(-model.log_temperatures)
  bins = model.bin_function(torch.stack([-thresholds, thresholds], dim=-1))
  matches = torch.einsum('btds,dcs->btdc', bins, model.bin_codes_1hot)
  weights = torch.prod(matches, dim=-2)
  return torch.einsum('bnd,ncd->bnc', weights, model.response).mean(dim=1)


def binary_loss(model_logits, labels, reduction='mean'):
  targets = torch.as_tensor(labels, dtype=model_logits.dtype, device=model_logits.device)
  return functional.binary_cross_entropy_with_logits(
    model_logits[:, 1] - model_logits[:, 0], targets, reduction=reduction)


def evaluate(model, features, labels, batch_size, device, mode, inverse_scale=None):
  model.eval()
  total = 0.0
  with torch.no_grad():
    for start in range(0, len(labels), batch_size):
      end = min(start + batch_size, len(labels))
      batch = make_batch(features, slice(start, end), device, mode, inverse_scale)
      total += binary_loss(logits(model, batch), labels[start:end], reduction='sum').item()
  loss = total / len(labels)
  if not math.isfinite(loss):
    raise ValueError('non-finite evaluation loss')
  return loss


def evaluate_dataset(model, dataset, batch_size, device, mode, inverse_scale):
  model.eval()
  total, rows, target_sum = 0.0, 0, 0.0
  with torch.no_grad():
    for features, labels in iter_batches(dataset, batch_size, len(inverse_scale)):
      batch = make_batch(features, slice(None), device, mode, inverse_scale)
      total += binary_loss(logits(model, batch), labels, reduction='sum').item()
      rows += len(labels)
      target_sum += float(labels.sum(dtype=np.float64))
  if not rows:
    raise ValueError('empty Dataset; get_rows() after None must restart the selection')
  loss = total / rows
  if not math.isfinite(loss):
    raise ValueError('non-finite evaluation loss')
  return {'logloss': loss, 'rows': rows, 'target_sum': target_sum,
          'mean_target': target_sum / rows}


def collect_initial(dataset, count, batch_size, n_features):
  features, labels, seen, exhausted = [], [], 0, False
  while seen < count:
    requested = min(count - seen, batch_size)
    rows = dataset.get_rows(requested)
    if rows is None:
      exhausted = True
      break
    x, y = validate_rows(rows, n_features, requested)
    features.append(x)
    labels.append(y)
    seen += len(y)
  if not seen:
    raise ValueError('empty training Dataset')
  return sparse.vstack(features, format='csr'), np.concatenate(labels), exhausted


def array_batches(features, labels, batch_size):
  for start in range(0, len(labels), batch_size):
    stop = min(start + batch_size, len(labels))
    yield features[start:stop], labels[start:stop]


def check_pass(current, previous, name):
  if previous is not None and (current['rows'] != previous['rows'] or not math.isclose(
      current['target_sum'], previous['target_sum'], rel_tol=1e-12, abs_tol=1e-12)):
    raise ValueError(name + ' Dataset changed between passes; return the same selection after None')


def resources(device):
  result = {'peak_rss_bytes': resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * 1024}
  if device.type == 'cuda':
    result.update(gpu_peak_allocated_bytes=torch.cuda.max_memory_allocated(device),
                  gpu_peak_reserved_bytes=torch.cuda.max_memory_reserved(device))
  return result


def _initialize(self, *, n_features=16384, num_trees=100, max_depth=6, feature_mode='vanga',
                features_per_node=None, epochs=10, patience=3, batch_size=1024,
                eval_batch_size=None, init_rows=1000, learning_rate=1e-3, seed=0,
                cpu_threads=4, device='cuda', input_mode='sparse', max_model_nodes=1000000,
                log_every_batches=1000, verbose=False):
  settings = dict(locals())
  del settings['self']
  self._settings = validate_settings(settings)
  self.model_ = self.scaler_ = self.metrics_ = self.device_ = self.output_dir_ = None


def _report(self, value):
  if self._settings['verbose']:
    print(json.dumps(value), flush=True)


def _fit(self, train, validation, output_dir):
  """Train on restarting Dataset streams, save artifacts to a new directory, return self."""
  for data in (train, validation):
    if isinstance(data, (str, PathLike)) and not Path(data).is_file():
      raise ValueError('input file does not exist: ' + str(data))
    if not isinstance(data, (Dataset, str, PathLike)):
      if not isinstance(data, tuple) or len(data) != 2:
        raise ValueError('expected a Dataset, LibSVM path or an (X, y) pair')
  args = SimpleNamespace(**self._settings)
  device = choose_device(args.device)
  torch.set_num_threads(args.cpu_threads)
  torch.manual_seed(args.seed)
  np.random.seed(args.seed)
  if device.type == 'cuda':
    torch.cuda.manual_seed_all(args.seed)
  output = Path(output_dir).resolve()
  output.mkdir(parents=True, exist_ok=False)
  self.model_ = self.scaler_ = None
  self.device_, self.output_dir_ = device, output
  started = time.monotonic()
  metadata = {'type': 'NODE', 'implementation': 'pytorch-tabular ODST',
              'n_features': args.n_features, 'num_trees': args.num_trees,
              'depth': args.max_depth, 'tree_output_dim': 2, 'tree_aggregation': 'mean logits',
              'feature_mode': args.feature_mode}
  if args.feature_mode == 'vanga':
    metadata.update(type='Vanga', implementation='PyTorch symmetric additive multi-branch trees',
                    features_per_node=args.features_per_node, split_sharing='per tree and level',
                    node_aggregation='sum of independently selected children')
  settings = dict(self._settings, output_dir=str(output))
  result = {'status': 'loading', 'model': metadata, 'settings': settings, 'device': str(device),
            'torch_version': torch.__version__,
            'pytorch_tabular_version': importlib.metadata.version('pytorch-tabular'), 'history': []}
  self.metrics_ = result
  result['checkpoint'] = str(output / 'node_checkpoint.pt')
  result['feature_scaler'] = str(output / 'feature_scaler.joblib')
  if device.type == 'cuda':
    result['gpu_name'] = torch.cuda.get_device_name(device)
  write_json(output / 'metrics.json', result)
  sources = []
  try:
    for data in (train, validation):
      sources.append(as_dataset(data, args.n_features))
    train_source, val_source = sources[0][0], sources[1][0]
    settings.update(train=describe_input(train), validation=describe_input(validation))
    initial_x, initial_y, initial_exhausted = collect_initial(
      train_source, args.init_rows, args.batch_size, args.n_features)
    scaler = StandardScaler(with_mean=False).fit(initial_x)
    result['preprocessing'] = {'scaler_fit_rows': len(initial_y),
                               'scaler_strategy': 'initial_train_rows',
                               'shuffle': 'within batches', 'dataset_cache': False}
    # Match the float32 transform used by exported NPZ consumers exactly.
    scaler.scale_ = np.asarray(scaler.scale_, dtype=np.float32)
    if not np.isfinite(scaler.scale_).all() or (scaler.scale_ <= 0).any():
      raise ValueError('invalid feature scales')
    inverse_scale = 1.0 / scaler.scale_
    if not np.isfinite(inverse_scale).all():
      raise ValueError('non-finite inverse feature scales')
    joblib.dump(scaler, output / 'feature_scaler.joblib')
    result['load_seconds'] = time.monotonic() - started
    model = make_model(args.n_features, args.num_trees, args.max_depth, args.feature_mode,
                       args.features_per_node)
    model.to(device)
    with torch.no_grad():
      initial = make_batch(initial_x, slice(None), device, 'dense', inverse_scale)
      if args.feature_mode == 'vanga':
        initialize_vanga(model, initial, initial_y)
      else:
        model(initial)
      del initial
    optimizer = torch.optim.Adam(model.parameters(), lr=args.learning_rate)
    rng = np.random.default_rng(args.seed)
    best_loss, best_epoch, best_state, stale = float('inf'), 0, None, 0
    fit_started = time.monotonic()
    result['status'] = 'training'
    train_stats = val_stats = None
    _report(self, {'event': 'initialized', 'initial_rows': len(initial_y), 'device': str(device),
                   'num_trees': args.num_trees, 'max_depth': args.max_depth,
                   'feature_mode': args.feature_mode})
    with (output / 'history.jsonl').open('w', buffering=1) as history:
      for epoch in range(1, args.epochs + 1):
        epoch_started = time.monotonic()
        model.train()
        batches = iter_batches(train_source, args.batch_size, args.n_features)
        if epoch == 1:
          remaining = () if initial_exhausted else batches
          batches = chain(array_batches(initial_x, initial_y, args.batch_size), remaining)
        running_loss, seen, target_sum = 0.0, 0, 0.0
        for batch_number, (features, labels) in enumerate(batches, 1):
          order = rng.permutation(len(labels))
          inputs = make_batch(features, order, device, args.input_mode, inverse_scale)
          optimizer.zero_grad(set_to_none=True)
          loss = binary_loss(logits(model, inputs), labels[order])
          value = loss.item()
          if not math.isfinite(value):
            raise ValueError('non-finite training loss')
          loss.backward()
          optimizer.step()
          running_loss += value * len(labels)
          seen += len(labels)
          target_sum += float(labels.sum(dtype=np.float64))
          if args.log_every_batches and batch_number % args.log_every_batches == 0:
            _report(self, {'event': 'batch', 'epoch': epoch, 'rows': seen,
                           'train_batch_logloss': running_loss / seen})
        if not seen:
          raise ValueError('training Dataset did not restart after None')
        current_train = {'rows': seen, 'target_sum': target_sum}
        check_pass(current_train, train_stats, 'training')
        train_stats = current_train
        current_val = evaluate_dataset(
          model, val_source, args.eval_batch_size, device, args.input_mode, inverse_scale)
        check_pass(current_val, val_stats, 'validation')
        val_stats = current_val
        val_loss = val_stats['logloss']
        result['rows'] = {'train': seen, 'validation': val_stats['rows'],
                          'train_target_sum': target_sum,
                          'validation_target_sum': val_stats['target_sum'],
                          'train_mean_target': target_sum / seen,
                          'validation_mean_target': val_stats['mean_target']}
        entry = {'epoch': epoch, 'train_batch_logloss': running_loss / seen,
                 'validation_logloss': val_loss, 'seconds': time.monotonic() - epoch_started}
        if val_loss < best_loss:
          best_loss, best_epoch, stale = val_loss, epoch, 0
          best_state = {k: v.detach().cpu().clone() for k, v in model.state_dict().items()}
          checkpoint = {'state_dict': {'trees.' + k: v for k, v in best_state.items()},
                        'model': metadata, 'best_epoch': best_epoch,
                        'validation_logloss': best_loss}
          partial = output / 'node_checkpoint.pt.partial'
          torch.save(checkpoint, partial)
          partial.replace(output / 'node_checkpoint.pt')
        else:
          stale += 1
        entry['best_epoch'] = best_epoch
        result['history'].append(entry)
        result.update(best_epoch=best_epoch, best_validation_logloss=best_loss,
                      epochs_completed=epoch, resources=resources(device))
        write_json(output / 'metrics.json', result)
        history.write(json.dumps(entry) + '\n')
        _report(self, entry)
        if epoch == 1:
          del initial_x, initial_y
        if args.patience and stale >= args.patience:
          break
    if best_state is None:
      raise RuntimeError('training did not produce a checkpoint')
    model.load_state_dict(best_state)
    result['training_seconds'] = time.monotonic() - fit_started
    for name, source, previous in (('train', train_source, train_stats),
                                    ('validation', val_source, val_stats)):
      current = evaluate_dataset(
        model, source, args.eval_batch_size, device, args.input_mode, inverse_scale)
      check_pass(current, previous, name)
      result[name + '_logloss'] = current['logloss']
    result['status'] = 'exporting'
    write_json(output / 'metrics.json', result)
    if args.feature_mode == 'vanga':
      result['export'] = export_vanga(model, scaler.scale_, output)
    else:
      result['export'] = export_catboost(
        model, scaler.scale_, train_source, val_source, output,
        args.eval_batch_size, args.cpu_threads, args.seed)
    if args.feature_mode != 'vanga':
      for name in ('train', 'validation'):
        if result['export']['datasets'][name]['rows'] != result['rows'][name]:
          raise ValueError(name + ' Dataset row count changed during export')
    result.update(status='complete', elapsed_seconds=time.monotonic() - started,
                  resources=resources(device))
    write_json(output / 'metrics.json', result)
    self.model_, self.scaler_ = model, scaler
    _report(self, result)
    return self
  except BaseException as error:
    result.update(status='failed', error_type=type(error).__name__, error=str(error),
                  elapsed_seconds=time.monotonic() - started, resources=resources(device))
    write_json(output / 'metrics.json', result)
    raise
  finally:
    for source, owned in sources:
      if owned:
        close_dataset(source)


def _require_fitted(self):
  if self.model_ is None or self.scaler_ is None:
    raise RuntimeError('VangaTrainer is not fitted; call fit() or load() first')


def _predict_raw(self, data, batch_size=None):
  """Return soft-checkpoint logits for raw LibSVM paths or dense/sparse feature matrices."""
  _require_fitted(self)
  count = self._settings['eval_batch_size'] if batch_size is None else validate_count(batch_size)
  if not isinstance(data, (Dataset, str, PathLike)):
    features = normalize_features(data, self._settings['n_features'], allow_empty=True)
    data = (features, np.zeros(features.shape[0], dtype=np.int64))
  predictions = []
  self.model_.eval()
  with torch.no_grad():
    for features, _ in iter_batches(data, count, self._settings['n_features']):
      inputs = make_batch(features, slice(None), self.device_, self._settings['input_mode'],
                           1.0 / self.scaler_.scale_)
      values = logits(self.model_, inputs)
      predictions.append((values[:, 1] - values[:, 0]).cpu().numpy().astype(np.float64))
  raw = np.concatenate(predictions) if predictions else np.empty(0, dtype=np.float64)
  if not np.isfinite(raw).all():
    raise ValueError('non-finite predictions')
  return raw


def _predict_proba(self, data, batch_size=None):
  """Return probabilities with columns [P(y=0), P(y=1)] for the soft checkpoint."""
  raw = self.predict_raw(data, batch_size=batch_size)
  positive = np.exp(-np.logaddexp(0, -raw))
  return np.column_stack((1 - positive, positive))


def _predict(self, data, batch_size=None):
  """Return binary labels using the soft checkpoint's highest-probability class."""
  return (self.predict_raw(data, batch_size=batch_size) > 0).astype(np.int64)


def _load(cls, output_dir, *, device='cpu', verbose=False):
  """Restore the best soft checkpoint and scaler from a completed fit's output directory."""
  output = Path(output_dir).resolve()
  metrics = json.loads((output / 'metrics.json').read_text())
  if metrics['status'] != 'complete':
    raise ValueError('output directory does not contain a completed training run')
  settings = {k: v for k, v in metrics['settings'].items()
              if k not in ('train', 'validation', 'output_dir')}
  settings.update(device=device, verbose=verbose)
  instance = cls(**settings)
  instance.device_ = choose_device(device)
  checkpoint = torch.load(output / 'node_checkpoint.pt', map_location='cpu', weights_only=True)
  args = SimpleNamespace(**instance._settings)
  model = make_model(args.n_features, args.num_trees, args.max_depth, args.feature_mode,
                     args.features_per_node)
  state = {k.removeprefix('trees.'): v for k, v in checkpoint['state_dict'].items()}
  model.load_state_dict(state)
  scaler = joblib.load(output / 'feature_scaler.joblib')
  if len(scaler.scale_) != args.n_features:
    raise ValueError('scaler feature count disagrees with the model configuration')
  instance.model_, instance.scaler_ = model.to(instance.device_), scaler
  instance.metrics_, instance.output_dir_ = metrics, output
  return instance


class VangaTrainer:
  """Train, predict and reload GPU tree models without argparse or subprocesses.

  Keyword-only construction accepts the CLI's training options with underscores:
  num_trees, max_depth, n_features, feature_mode, features_per_node, epochs,
  patience, batch_size, eval_batch_size, init_rows, learning_rate, seed,
  cpu_threads, device, input_mode, max_model_nodes and log_every_batches.
  The Python default feature_mode is 'vanga'; verbose=False suppresses progress output.

  fit(train, validation, output_dir) accepts Dataset objects and returns self.
  A Dataset's call after None must restart the same selection. No disk cache is used.
  File paths and (X, y) pairs remain supported via adapters. Initialization retains
  at most init_rows records; scaler statistics are frozen after these first rows.
  model_, scaler_, metrics_, device_ and output_dir_ expose the fitted state.
  predict_raw/proba/predict use the soft best checkpoint on original input features.
  The hardened model is exported by fit; its metrics are in metrics_['export'].
  Implementations are outside the class body, following the project's source style.
  """

  __init__ = _initialize
  fit = _fit
  predict_raw = _predict_raw
  predict_proba = _predict_proba
  predict = _predict
  load = classmethod(_load)


def train(args):
  """Compatibility entry point for the CLI; returns the fitted trainer's metrics."""
  settings = vars(args).copy()
  train_data, validation = settings.pop('train'), settings.pop('validation')
  output = settings.pop('output_dir')
  settings['verbose'] = True
  return VangaTrainer(**settings).fit(train_data, validation, output).metrics_

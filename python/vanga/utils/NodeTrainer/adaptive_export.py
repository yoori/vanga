"""Hard Vanga export and CPU scoring/pruning for adaptive topologies."""

import json
import math
import time

import numpy as np
import torch

from ...dataset import iter_batches
from .export import raw_float32_borders


def split_metadata(model, scale):
  splits = [split for _, split in model.iter_splits()]
  if not splits:
    return {}, 0
  with torch.no_grad():
    columns = np.asarray([int(torch.argmax(split.feature_logits).item()) for split in splits],
                         dtype=np.int64)
    thresholds = np.asarray([float(split.threshold.detach().cpu()) for split in splits],
                            dtype=np.float32)
    usages = np.asarray([float(split.usage.detach().cpu()) for split in splits],
                        dtype=np.float64)
  if not np.isfinite(thresholds).all():
    raise ValueError('cannot export non-finite split thresholds')
  if not np.isfinite(usages).all():
    raise ValueError('cannot export non-finite split usage coefficients')
  inverse_scale = np.asarray(1.0 / scale, dtype=np.float32)
  borders = raw_float32_borders(thresholds, inverse_scale[columns])
  return {id(split): (int(column), float(border), float(usage))
          for split, column, border, usage in zip(splits, columns, borders, usages)}, len(splits)


def export_adaptive_vanga(model, scale, output, filename='node_hardened.vanga'):
  """Write an arbitrary container graph as a numeric DTree model."""
  started = time.monotonic()
  metadata, split_count = split_metadata(model, scale)
  containers = model.iter_containers()
  ids = {id(container): index + 1 for index, container in enumerate(containers)}
  scales = {id(model.root): 1.0}
  for container in containers:
    parent_scale = scales[id(container)]
    for split in container.splits:
      child_scale = parent_scale * float(split.usage.detach().cpu())
      scales[id(split.yes)] = child_scale
      scales[id(split.no)] = child_scale
  path = output / filename
  partial = path.with_name(path.name + '.partial')
  with partial.open('w') as target:
    target.write('dtree-float\n')
    for container in containers:
      value = float(container.value.detach().cpu()) * scales[id(container)]
      if not math.isfinite(value):
        raise ValueError('cannot export non-finite container value')
      branches = []
      for split in container.splits:
        feature, border, _usage = metadata[id(split)]
        branches.append('{}:{}:{}:{:.17g}:0'.format(
          feature + 1, ids[id(split.yes)], ids[id(split.no)], border))
      target.write('{}\t{:.17g}\t{}\n'.format(
        ids[id(container)], value, '|'.join(branches)))
  partial.replace(path)
  result = {'is_lossless': False,
            'note': 'NODE soft gates are hardened at their learned thresholds.',
            'feature_mode': 'adaptive-vanga', 'input_space': 'raw_libsvm',
            'input_features': model.n_features, 'input_projection_features': 0,
            'vanga_nodes': split_count, 'container_nodes': len(containers),
            'nodes': len(containers), 'native_model': str(path),
            'elapsed_seconds': time.monotonic() - started}
  (output / (filename + '.json')).write_text(json.dumps(result, indent=2) + '\n')
  return result


def _hard_batch(model, features, scale):
  """Return hard Vanga scores and each split's full path-weighted contribution."""
  paths_and_splits = model.iter_splits()
  splits = [split for _, split in paths_and_splits]
  if not splits:
    rows = features.shape[0]
    return np.full(rows, float(model.root.value.detach().cpu()), dtype=np.float64), {}

  metadata, _ = split_metadata(model, scale)
  columns = sorted({metadata[id(split)][0] for split in splits})
  column_indexes = {column: index for index, column in enumerate(columns)}
  values = features[:, columns].toarray().astype(np.float32, copy=False)
  conditions = {
    id(split): values[:, column_indexes[metadata[id(split)][0]]] > metadata[id(split)][1]
    for split in splits
  }
  rows = features.shape[0]
  container_values = {}

  def evaluate(container):
    cached = container_values.get(id(container))
    if cached is not None:
      return cached
    result = np.full(rows, float(container.value.detach().cpu()), dtype=np.float64)
    for split in container.splits:
      decision = conditions[id(split)]
      result += metadata[id(split)][2] * np.where(
        decision, evaluate(split.yes), evaluate(split.no))
    container_values[id(container)] = result
    return result

  scores = evaluate(model.root)
  contributions = {}

  def collect(container, active, prefix=()):
    for index, split in enumerate(container.splits):
      path = prefix + (index,)
      decision = conditions[id(split)]
      yes_values = evaluate(split.yes)
      no_values = evaluate(split.no)
      usage = metadata[id(split)][2]
      contributions[path] = active * usage * np.where(decision, yes_values, no_values)
      collect(split.yes, active * usage * decision, path + (1,))
      collect(split.no, active * usage * ~decision, path + (0,))

  collect(model.root, np.ones(rows, dtype=bool))
  return scores, contributions


def _logloss_sum(logits, labels):
  logits = np.asarray(logits, dtype=np.float64)
  labels = np.asarray(labels, dtype=np.float64)
  return np.maximum(logits, 0.0) - logits * labels + np.log1p(np.exp(-np.abs(logits)))


def evaluate_hard_vanga(model, dataset, scale, batch_size, n_features):
  total, rows = 0.0, 0
  for features, labels in iter_batches(dataset, batch_size, n_features):
    scores, _ = _hard_batch(model, features.tocsr(copy=False), scale)
    total += float(_logloss_sum(scores, labels).sum())
    rows += len(labels)
  if not rows:
    raise ValueError('empty prediction dataset')
  return {'logloss': total / rows, 'rows': rows}


def prune_vanga(model, dataset, scale, target_nodes, batch_size, n_features, epsilon=1e-6):
  """Greedily prune within the node budget while loss increase stays within epsilon."""
  remaining = int(target_nodes)
  removed = 0
  history = []
  next_delta = None
  while remaining > 0 and model.branch_count():
    paths = [path for path, _ in model.iter_splits()]
    losses = np.zeros(len(paths), dtype=np.float64)
    baseline = 0.0
    rows = 0
    path_indexes = {path: index for index, path in enumerate(paths)}
    for features, labels in iter_batches(dataset, batch_size, n_features):
      scores, contributions = _hard_batch(model, features.tocsr(copy=False), scale)
      baseline += float(_logloss_sum(scores, labels).sum())
      rows += len(labels)
      matrix = np.column_stack([contributions[path] for path in paths])
      candidate_logits = scores[:, None] - matrix
      losses += _logloss_sum(candidate_logits, labels[:, None]).sum(axis=0)
    if not rows:
      raise ValueError('empty pruning dataset')
    deltas = losses / rows - baseline / rows
    selected_index = int(np.argmin(deltas))
    selected_delta = float(deltas[selected_index])
    if selected_delta > epsilon:
      next_delta = selected_delta
      break
    selected_path = paths[selected_index]
    _, split = next(item for item in model.iter_splits() if item[0] == selected_path)
    removed_subtree = model.subtree_split_count(split)
    model.remove_split(selected_path)
    remaining -= removed_subtree
    removed += removed_subtree
    history.append({'path': list(selected_path), 'removed_nodes': removed_subtree,
                    'delta_logloss': selected_delta, 'remaining_target': remaining})
  return {'requested_nodes': int(target_nodes), 'removed_nodes': removed,
          'overshoot_nodes': max(0, removed - int(target_nodes)), 'epsilon': float(epsilon),
          'stopped_early': next_delta is not None,
          'next_best_delta_logloss': next_delta, 'history': history}

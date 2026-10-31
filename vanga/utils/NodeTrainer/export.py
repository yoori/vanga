"""Harden ODST gates and export the scalar model accepted by Vanga::DTree."""

import json
import math
import time

import numpy as np
from scipy import sparse
from catboost import CatBoostClassifier, __version__ as catboost_version
import torch

from ...dataset import iter_batches


def raw_float32_borders(borders, inverse_scale):
  borders, inverse_scale = np.broadcast_arrays(np.asarray(borders, dtype=np.float32),
                                               np.asarray(inverse_scale, dtype=np.float32))
  if not np.isfinite(borders).all() or not np.isfinite(inverse_scale).all():
    raise ValueError('non-finite border or inverse scale')
  if (inverse_scale <= 0).any():
    raise ValueError('inverse scales must be positive')
  # Ordered float32 bit keys permit finding the exact raw cut despite normalization rounding.
  minimum, maximum = 0x00800000, 0xff7fffff
  low = np.full(borders.shape, minimum, dtype=np.int64)
  high = np.full(borders.shape, maximum + 1, dtype=np.int64)
  with np.errstate(over='ignore', under='ignore', invalid='ignore'):
    while np.any(low < high):
      active = low < high
      middle = (low + high) // 2
      bits = np.where(middle >= 0x80000000, middle & 0x7fffffff, ~middle & 0xffffffff)
      values = bits.astype(np.uint32).view(np.float32)
      greater = (values * inverse_scale) > borders
      high = np.where(active & greater, middle, high)
      low = np.where(active & ~greater, middle + 1, low)
  if np.any(low == minimum):
    raise ValueError('normalized split has no finite raw float32 boundary')
  keys = low - 1
  bits = np.where(keys >= 0x80000000, keys & 0x7fffffff, ~keys & 0xffffffff)
  return bits.astype(np.uint32).view(np.float32)


def export_catboost(layer, scale, train, validation, output, batch_size, threads, seed):
  started = time.monotonic()
  num_trees, depth = layer.num_trees, layer.depth
  with torch.no_grad():
    selectors = layer.choice_function(layer.feature_selection_logits, dim=0)
    selectors = selectors.reshape(len(scale), num_trees * depth).detach().cpu().numpy()
  selectors = np.asarray(selectors, dtype=np.float32)
  thresholds = layer.feature_thresholds.detach().cpu().numpy().astype(np.float32)
  responses = layer.response.detach().cpu().numpy().astype(np.float64)
  if not all(np.isfinite(x).all() for x in (selectors, thresholds, responses)):
    raise ValueError('cannot export non-finite model parameters')
  single = getattr(layer, 'feature_mode', 'projection') == 'single'
  inverse_scale = np.asarray(1.0 / scale, dtype=np.float32)
  if single:
    if not np.all((selectors == 0) | (selectors == 1)) or not np.all(selectors.sum(axis=0) == 1):
      raise ValueError('single-feature export requires exact one-hot selectors')
    columns = selectors.argmax(axis=0).reshape(num_trees, depth)
    split_borders = raw_float32_borders(thresholds, inverse_scale[columns])
    input_features = len(scale)
  else:
    columns = np.arange(num_trees * depth).reshape(num_trees, depth)
    split_borders = thresholds
    input_features = num_trees * depth
  rng = np.random.default_rng(seed)
  template = CatBoostClassifier(iterations=1, depth=1, loss_function='Logloss',
                               thread_count=threads, verbose=False, allow_writing_files=False,
                               random_seed=seed)
  template.fit(rng.normal(size=(32, input_features)), np.arange(32) % 2)
  path = output / 'node_hardened_catboost.json'
  partial = path.with_name(path.name + '.partial')
  template.save_model(str(partial), format='json')
  model = json.loads(partial.read_text())
  features, split_indexes = [], {}
  border_offset = 0
  for feature_index, column in enumerate(sorted(set(columns.reshape(-1).tolist()))):
    borders = sorted(set(float(b) for b in split_borders[columns == column]))
    name = 'libsvm_feature_{}'.format(column + 1) if single else (
      'node_tree_{:03d}_level_{}'.format(column // depth, column % depth))
    features.append({'feature_index': feature_index, 'flat_feature_index': column,
                     'feature_id': name, 'borders': borders,
                     'has_nans': False, 'nan_value_treatment': 'AsIs'})
    for index, border in enumerate(borders):
      split_indexes[(column, border)] = (feature_index, border_offset + index)
    border_offset += len(borders)
  model['features_info'] = {'float_features': features}
  # ODST's low/high branch polarity is opposite to CatBoost's leaf-index bit.
  leaf_order = np.arange(1 << depth) ^ ((1 << depth) - 1)
  leaf_values = (responses[:, 1] - responses[:, 0])[:, leaf_order] / num_trees
  trees = []
  for tree in range(num_trees):
    splits = []
    for level in range(depth):
      column, border = int(columns[tree, level]), float(split_borders[tree, level])
      feature_index, split_index = split_indexes[(column, border)]
      splits.append({'split_type': 'FloatFeature', 'float_feature_index': feature_index,
                     'split_index': split_index, 'border': border})
    trees.append({'splits': splits, 'leaf_values': leaf_values[tree].tolist(),
                  'leaf_weights': [1.0] * (1 << depth)})
  model['oblivious_trees'] = trees
  model['scale_and_bias'] = [1, [0.0]]
  params = model['model_info']['params']
  params['boosting_options']['iterations'] = num_trees
  params['tree_learner_options']['depth'] = depth
  partial.write_text(json.dumps(model, separators=(',', ':'), allow_nan=False) + '\n')
  exported = CatBoostClassifier()
  exported.load_model(str(partial), format='json')
  sparse_selectors = sparse.csr_matrix(selectors)
  checks = {}
  for name, dataset in (('train', train), ('validation', validation)):
    total_loss, max_difference, rows = 0.0, 0.0, 0
    for batch, labels in iter_batches(dataset, batch_size, len(scale)):
      if single:
        projected = batch[:, columns.reshape(-1)].toarray().astype(np.float32)
        projected *= inverse_scale[columns.reshape(-1)]
        prediction_input = batch
      else:
        batch.data *= inverse_scale[batch.indices]
        if not np.isfinite(batch.data).all():
          raise ValueError('non-finite scaled feature values')
        projected = (batch @ sparse_selectors).toarray().astype(np.float32)
        prediction_input = projected
      raw = np.asarray(exported.predict(prediction_input, prediction_type='RawFormulaVal',
                                       thread_count=threads)).reshape(-1)
      bits = projected.reshape(-1, num_trees, depth) > thresholds
      indexes = (bits * (1 << np.arange(depth))).sum(axis=-1)
      reference = leaf_values[np.arange(num_trees)[None, :], indexes].sum(axis=1)
      difference = float(np.max(np.abs(raw - reference)))
      max_difference = max(max_difference, difference)
      if not np.isfinite(raw).all() or not np.allclose(raw, reference, atol=1e-12, rtol=1e-12):
        raise ValueError('CatBoost export disagrees with independent leaf reconstruction')
      total_loss += np.sum(np.logaddexp(0, raw) - labels * raw)
      rows += len(labels)
    if not rows:
      raise ValueError('empty Dataset during export; get_rows() after None must restart')
    loss = float(total_loss / rows)
    if not math.isfinite(loss):
      raise ValueError('non-finite hardened model loss')
    checks[name] = {'rows': rows, 'logloss': loss, 'max_abs_raw_difference': max_difference}
  partial.replace(path)
  transform = None
  if not single:
    transform = output / 'node_projection_transform.npz'
    with transform.with_name(transform.name + '.partial').open('wb') as target:
      np.savez_compressed(target, scaler_scale=scale, selectors=selectors, thresholds=thresholds,
                          num_trees=num_trees, depth=depth)
    transform.with_name(transform.name + '.partial').replace(transform)
  result = {'is_lossless': False, 'note': 'NODE soft gates are hardened at learned thresholds.',
            'num_trees': num_trees, 'depth': depth, 'input_features': input_features,
            'feature_mode': 'single' if single else 'projection',
            'input_space': 'raw_libsvm' if single else 'node_projections',
            'input_projection_features': 0 if single else num_trees * depth,
            'selected_feature_ids': (columns + 1).tolist() if single else None,
            'catboost_version': catboost_version, 'datasets': checks,
            'json_model': str(path), 'projection_transform': str(transform) if transform else None,
            'elapsed_seconds': time.monotonic() - started}
  (output / 'export_metrics.json').write_text(json.dumps(result, indent=2) + '\n')
  return result

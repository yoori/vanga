"""Configuration validation shared by the Python API and command-line utility."""

import math
from numbers import Integral, Real


def validate_settings(settings):
  result = dict(settings)
  positive = ('n_features', 'num_trees', 'max_depth', 'epochs', 'batch_size', 'init_rows',
              'cpu_threads', 'max_model_nodes')
  nonnegative = ('patience', 'seed', 'log_every_batches')
  optional = ('features_per_node', 'eval_batch_size')
  for name in positive + nonnegative + optional:
    value = result[name]
    if name in optional and value is None:
      continue
    minimum = 0 if name in nonnegative else 1
    if isinstance(value, bool) or not isinstance(value, Integral) or value < minimum:
      raise ValueError('{} must be an integer >= {}'.format(name, minimum))
    result[name] = int(value)
  if result['max_depth'] > 16:
    raise ValueError('max_depth must be in [1, 16]')
  rate = result['learning_rate']
  if isinstance(rate, bool) or not isinstance(rate, Real) or not math.isfinite(rate) or rate <= 0:
    raise ValueError('learning_rate must be finite and positive')
  result['learning_rate'] = float(rate)
  mode = result['feature_mode']
  if mode not in ('projection', 'single', 'vanga'):
    raise ValueError('feature_mode must be projection, single or vanga')
  if result['input_mode'] not in ('sparse', 'dense'):
    raise ValueError('input_mode must be sparse or dense')
  if not isinstance(result['device'], str) or not result['device']:
    raise ValueError('device must be a nonempty string: cpu, cuda or cuda:INDEX')
  if result['features_per_node'] is not None and mode != 'vanga':
    if result['features_per_node'] != 1:
      raise ValueError('features_per_node requires feature_mode=vanga')
  result['features_per_node'] = result['features_per_node'] or (2 if mode == 'vanga' else 1)
  result['eval_batch_size'] = result['eval_batch_size'] or result['batch_size']
  if mode == 'vanga':
    if result['features_per_node'] > result['n_features']:
      raise ValueError('features_per_node cannot exceed n_features')
    width = 2 * result['features_per_node']
    nodes = result['num_trees'] * ((width ** (result['max_depth'] + 1) - 1) // (width - 1))
    if nodes > result['max_model_nodes']:
      raise ValueError('vanga topology requires {} nodes; reduce trees/depth/features_per_node '
                        'or explicitly raise max_model_nodes'.format(nodes))
  if 'verbose' in result and not isinstance(result['verbose'], bool):
    raise ValueError('verbose must be a bool')
  return result

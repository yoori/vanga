"""Harden additive multi-branch trees directly into the native numeric DTree format."""

import json
import time

import numpy as np
import torch

from .export import raw_float32_borders
from .multibranch import branch_selectors


def export_vanga(model, scale, output):
  started = time.monotonic()
  trees, depth, branches = model.num_trees, model.depth, model.features_per_node
  with torch.no_grad():
    columns = branch_selectors(model.feature_selection_logits).argmax(dim=0).cpu().numpy()
  thresholds = model.feature_thresholds.detach().cpu().numpy().astype(np.float32)
  responses = model.response.detach().cpu().numpy().astype(np.float64) / trees
  bias = float(model.bias.detach().cpu())
  parameters = (model.feature_selection_logits, model.log_temperatures)
  if not all(torch.isfinite(value).all().item() for value in parameters):
    raise ValueError('cannot export non-finite model parameters')
  if not all(np.isfinite(value).all() for value in (thresholds, responses, bias)):
    raise ValueError('cannot export non-finite model parameters')
  inverse_scale = np.asarray(1.0 / scale, dtype=np.float32)
  borders = raw_float32_borders(thresholds, inverse_scale[columns])
  width = 2 * branches
  offsets = [(width ** level - 1) // (width - 1) for level in range(depth + 2)]
  nodes_per_tree = offsets[depth + 1]

  def node_id(tree, level, position):
    return 2 + tree * (nodes_per_tree - 1) + offsets[level] - 1 + position

  def branch_text(tree, level, position):
    checks = []
    for branch in range(branches):
      yes = node_id(tree, level + 1, position * width + 2 * branch + 1)
      no = node_id(tree, level + 1, position * width + 2 * branch)
      feature = int(columns[tree, level, branch]) + 1
      border = float(borders[tree, level, branch])
      checks.append('{}:{}:{}:{:.17g}:0'.format(feature, yes, no, border))
    return '|'.join(checks)

  path = output / 'node_hardened.vanga'
  partial = path.with_name(path.name + '.partial')
  with partial.open('w') as target:
    root_checks = '|'.join(branch_text(tree, 0, 0) for tree in range(trees))
    target.write('dtree-float\n1\t{:.17g}\t{}\n'.format(bias, root_checks))

    def write_node(tree, level, position):
      index = node_id(tree, level, position)
      if level == depth:
        target.write('{}\t{:.17g}\t\n'.format(index, responses[tree, position]))
        return
      target.write('{}\t0\t{}\n'.format(index, branch_text(tree, level, position)))
      for branch in range(branches):
        write_node(tree, level + 1, position * width + 2 * branch + 1)
        write_node(tree, level + 1, position * width + 2 * branch)

    for tree in range(trees):
      for branch in range(branches):
        write_node(tree, 1, 2 * branch + 1)
        write_node(tree, 1, 2 * branch)
  partial.replace(path)
  result = {'is_lossless': False,
            'note': 'Soft gates are hardened at learned thresholds; verify with DTreeTrainer.',
            'num_trees': trees, 'depth': depth, 'features_per_node': branches,
            'feature_mode': 'vanga', 'input_space': 'raw_libsvm',
            'input_features': len(scale), 'input_projection_features': 0,
            'selected_feature_ids': (columns + 1).tolist(),
            'nodes': 1 + trees * (nodes_per_tree - 1), 'native_model': str(path),
            'projection_transform': None, 'elapsed_seconds': time.monotonic() - started}
  (output / 'export_metrics.json').write_text(json.dumps(result, indent=2) + '\n')
  return result

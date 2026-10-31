"""Symmetric additive Vanga nodes with learned, discrete feature selection."""

import math

import numpy as np
import torch
from pytorch_tabular.models.common.layers import activations


def branch_selectors(scores):
  mask = torch.zeros_like(scores[..., 0], dtype=torch.bool)
  selectors = []
  for branch in range(scores.shape[-1]):
    values = scores[..., branch].masked_fill(mask, -torch.inf)
    soft = activations.entmax15(values, dim=0)
    chosen = values.argmax(dim=0, keepdim=True)
    hard = torch.zeros_like(soft).scatter_(0, chosen, 1)
    # Forward selects one raw feature; entmax supplies its surrogate gradient.
    selectors.append(hard + (soft - soft.detach()))
    mask = mask.scatter(0, chosen, True)
  return torch.stack(selectors, dim=-1)


def make_vanga_model(n_features, num_trees, depth, features_per_node):
  if not 1 <= features_per_node <= n_features:
    raise ValueError('features per node must be between 1 and n_features')
  model = torch.nn.Module()
  model.feature_mode = 'vanga'
  model.num_trees, model.depth = num_trees, depth
  model.features_per_node = features_per_node
  model.feature_selection_logits = torch.nn.Parameter(
    torch.randn(n_features, num_trees, depth, features_per_node))
  shape = (num_trees, depth, features_per_node)
  model.feature_thresholds = torch.nn.Parameter(torch.zeros(shape))
  model.log_temperatures = torch.nn.Parameter(torch.zeros(shape))
  leaves = (2 * features_per_node) ** depth
  model.response = torch.nn.Parameter(
    torch.randn(num_trees, leaves) * (0.01 / math.sqrt(features_per_node ** depth)))
  model.bias = torch.nn.Parameter(torch.zeros(()))
  return model


def branch_values(model, inputs):
  selectors = branch_selectors(model.feature_selection_logits)
  selectors = selectors.reshape(inputs.shape[1], -1).contiguous()
  values = inputs @ selectors
  return values.reshape(inputs.shape[0], model.num_trees, model.depth, model.features_per_node)


def initialize_vanga(model, inputs, labels):
  with torch.no_grad():
    values = branch_values(model, inputs)
    sorted_values = values.sort(dim=0).values
    rows = len(values)
    indexes = torch.randint(max(1, rows // 5), max(2, 4 * rows // 5),
                            model.feature_thresholds.shape, device=inputs.device)
    indexes.clamp_(max=rows - 1)
    thresholds = sorted_values.gather(0, indexes.unsqueeze(0)).squeeze(0)
    spread = sorted_values[3 * (rows - 1) // 4] - sorted_values[(rows - 1) // 4]
    model.feature_thresholds.copy_(thresholds)
    model.log_temperatures.copy_(spread.clamp_min(0.1).log())
    prior = (float(np.sum(labels)) + 0.5) / (len(labels) + 1)
    model.bias.fill_(math.log(prior / (1 - prior)))


def vanga_logits(model, inputs):
  values = branch_values(model, inputs)
  margins = (values - model.feature_thresholds) * torch.exp(-model.log_temperatures)
  gates = activations.entmoid15(margins)
  children = model.response.unsqueeze(0)
  for level in reversed(range(model.depth)):
    # Every parent sums K independently gated child pairs.
    children = children.reshape(children.shape[0], model.num_trees, -1,
                                model.features_per_node, 2)
    gate = gates[:, :, level, None, :]
    children = (children[..., 0] + gate * (children[..., 1] - children[..., 0])).sum(dim=-1)
  raw = children.squeeze(-1).mean(dim=1) + model.bias
  return torch.stack((torch.zeros_like(raw), raw), dim=-1)


def hard_vanga_raw(bits, responses, bias):
  """Float64 reference with the summation order of the merged native DTree root."""
  rows, trees, depth, branches = bits.shape
  children = np.asarray(responses, dtype=np.float64)[None, :, :]
  for level in reversed(range(1, depth)):
    children = children.reshape(children.shape[0], trees, -1, branches, 2)
    selected = np.where(bits[:, :, level, None, :], children[..., 1], children[..., 0])
    children = np.zeros(selected.shape[:-1], dtype=np.float64)
    for branch in range(branches):
      children += selected[..., branch]
  children = children.reshape(children.shape[0], trees, branches, 2)
  raw = np.full(rows, bias, dtype=np.float64)
  for tree in range(trees):
    for branch in range(branches):
      raw += np.where(bits[:, tree, 0, branch], children[:, tree, branch, 1],
                      children[:, tree, branch, 0])
  return raw

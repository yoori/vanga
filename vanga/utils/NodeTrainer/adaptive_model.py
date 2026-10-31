"""NODE module for arbitrary additive Vanga container topologies."""

import math

import numpy as np
import torch
from torch import nn
from pytorch_tabular.models.common.layers import activations


class VangaContainer(nn.Module):
  """One DTree node: an unconditional value plus additive yes/no splits."""

  def __init__(self, n_features, value=0.0, device=None):
    super().__init__()
    self.value = nn.Parameter(torch.tensor(float(value), dtype=torch.float32, device=device))
    self.splits = nn.ModuleList()
    self.n_features = n_features


class VangaSplit(nn.Module):
  """One numeric feature test whose outcomes point to two containers."""

  def __init__(self, n_features, device=None):
    super().__init__()
    self.feature_logits = nn.Parameter(torch.randn(n_features, device=device))
    self.threshold = nn.Parameter(torch.zeros((), dtype=torch.float32, device=device))
    self.log_temperature = nn.Parameter(torch.zeros((), dtype=torch.float32, device=device))
    self.usage = nn.Parameter(torch.ones((), dtype=torch.float32, device=device))
    self.yes = VangaContainer(n_features, device=device)
    self.no = VangaContainer(n_features, device=device)


class AdaptiveVangaModel(nn.Module):
  """A dynamically mutable NODE graph with Vanga's additive container semantics."""

  def __init__(self, n_features, device=None):
    super().__init__()
    self.n_features = int(n_features)
    self.feature_mode = 'adaptive-vanga'
    self.root = VangaContainer(n_features, device=device)

  def iter_containers(self):
    result = []

    def visit(container):
      result.append(container)
      for split in container.splits:
        visit(split.yes)
        visit(split.no)

    visit(self.root)
    return result

  def iter_splits(self):
    result = []

    def visit(container, path=()):
      for index, split in enumerate(container.splits):
        split_path = path + (index,)
        result.append((split_path, split))
        visit(split.yes, split_path + (1,))
        visit(split.no, split_path + (0,))

    visit(self.root)
    return result

  def branch_count(self):
    return len(self.iter_splits())

  def add_random_splits(self, count, rng, device=None):
    """Insert splits sequentially, sampling uniformly from all current containers."""
    containers = self.iter_containers()
    added = []
    for _ in range(count):
      container = containers[int(rng.integers(len(containers)))]
      split = VangaSplit(self.n_features, device=device)
      container.splits.append(split)
      containers.extend((split.yes, split.no))
      added.append(split)
    return added

  def remove_split(self, path):
    container = self.root
    for index, outcome in zip(path[:-1:2], path[1:-1:2]):
      split = container.splits[index]
      container = split.yes if outcome else split.no
    del container.splits[path[-1]]

  @staticmethod
  def subtree_split_count(split):
    def count(container):
      return sum(1 + count(branch.yes) + count(branch.no) for branch in container.splits)
    return 1 + count(split.yes) + count(split.no)

  def initialize_added_splits(self, splits, initial_features, rng):
    """Give fresh splits data-aware borders and small random leaf values."""
    values = initial_features.toarray() if hasattr(initial_features, 'toarray') else initial_features
    values = np.asarray(values, dtype=np.float32)
    if not len(values):
      return
    with torch.no_grad():
      for split in splits:
        feature = int(torch.argmax(split.feature_logits).item())
        column = values[:, feature]
        low, high = np.quantile(column, (0.2, 0.8))
        split.threshold.copy_(torch.tensor(rng.uniform(low, high), device=split.threshold.device))
        spread = np.quantile(column, 0.75) - np.quantile(column, 0.25)
        split.log_temperature.copy_(torch.tensor(math.log(max(float(spread), 0.1)),
                                                   device=split.log_temperature.device))
        split.yes.value.copy_(torch.tensor(rng.normal(0.0, 0.01), device=split.yes.value.device))
        split.no.value.copy_(torch.tensor(rng.normal(0.0, 0.01), device=split.no.value.device))

  def forward(self, inputs, hard=False):
    splits = [split for _, split in self.iter_splits()]
    if not splits:
      return self.root.value.expand(inputs.shape[0])
    logits = torch.stack([split.feature_logits for split in splits], dim=1)
    if hard:
      columns = logits.argmax(dim=0)
      selectors = torch.nn.functional.one_hot(columns, self.n_features).to(inputs.dtype).T
    else:
      soft = activations.entmax15(logits, dim=0)
      selected = logits.argmax(dim=0, keepdim=True)
      one_hot = torch.zeros_like(soft).scatter_(0, selected, 1.0)
      selectors = (one_hot + soft - soft.detach()).to(inputs.dtype)
    if inputs.layout == torch.strided:
      feature_values = inputs @ selectors
    else:
      feature_values = torch.sparse.mm(inputs, selectors.contiguous())
    positions = {id(split): index for index, split in enumerate(splits)}
    conditions = {}
    for split in splits:
      value = feature_values[:, positions[id(split)]]
      if hard:
        conditions[id(split)] = value > split.threshold
      else:
        conditions[id(split)] = activations.entmoid15(
          (value - split.threshold) * torch.exp(-split.log_temperature))

    def evaluate(container):
      result = container.value.expand(inputs.shape[0])
      for split in container.splits:
        gate = conditions[id(split)]
        yes = evaluate(split.yes)
        no = evaluate(split.no)
        result = result + split.usage * (gate * yes + (1.0 - gate) * no)
      return result

    return evaluate(self.root)

  def topology(self):
    def encode(container):
      return {'splits': [{'yes': encode(split.yes), 'no': encode(split.no)}
                         for split in container.splits]}
    return encode(self.root)

  @classmethod
  def from_topology(cls, n_features, topology, device=None):
    model = cls(n_features, device=device)

    def populate(container, spec):
      for split_spec in spec.get('splits', []):
        split = VangaSplit(n_features, device=device)
        container.splits.append(split)
        populate(split.yes, split_spec['yes'])
        populate(split.no, split_spec['no'])

    populate(model.root, topology)
    return model

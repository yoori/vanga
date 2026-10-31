Vanga Python package
====================

Install from the repository root with ``python -m pip install .`` (or ``-e .``
for development). Python 3.10+ is required. Install the PyTorch build appropriate
for your CUDA driver before installing Vanga if training on a GPU.

Training from LibSVM
-------------------

.. code-block:: python

  from vanga import LibSvmFileDataset, VangaTrainer

  with LibSvmFileDataset("train.libsvm.gz", n_features=16384) as train:
    with LibSvmFileDataset("validation.libsvm.gz", n_features=16384) as validation:
      trainer = VangaTrainer(
        n_features=16384, num_trees=100, max_depth=6,
        features_per_node=2, device="cuda",
      )
      trainer.fit(train, validation, output_dir="new-run")

  print(trainer.metrics_["validation_logloss"])
  probabilities = trainer.predict_proba("validation.libsvm.gz")
  restored = VangaTrainer.load("new-run", device="cpu")

Adaptive single-tree training
-----------------------------

``AdaptiveVangaTrainer`` grows one irregular Vanga tree. Primary train updates
NODE parameters, secondary train stops each NODE fit and drives CPU pruning,
and validation selects the best hardened Vanga checkpoint.

.. code-block:: python

  from vanga import AdaptiveVangaTrainer

  trainer = AdaptiveVangaTrainer(n_features=16384, device="cuda")
  trainer.fit(primary_train, secondary_train, validation, output_dir="adaptive-run")

The command-line entry point is ``vanga-adaptive-train``. Its first step is an
extend step; defaults are 15 replacement nodes, 5 extra extend nodes, 5% relative
improvement, and patience 5 for topology steps. Each split is inserted into a
uniformly selected container. Artifacts for every step are written under
``steps/``; ``best_node_model.pt`` and ``best_vanga_model.vanga`` point to the
best validation step.

``fit`` returns the trainer. The output directory must be new. The default
Python mode is ``vanga``: each node adds contributions from several independent
feature checks, with shared checks for nodes on the same tree level.
``feature_mode="single"`` and ``"projection"`` retain the ODST variants.
The CLI is available as ``vanga-train --help``; its default mode remains
``projection``. Set ``--feature-mode vanga --features-per-node 2`` for Vanga nodes.

Dataset contract
----------------

Subclass ``Dataset`` and implement ``get_rows(count)``. This is the only required
method. Each call returns either:

* ``(X, y)`` containing between 1 and ``count`` rows;
* ``None`` after all rows in the current traversal have been returned.

The call after ``None`` must start the same selection from the beginning.
Short batches do not terminate a traversal. Empty batches are invalid: use
``None``. Each traversal must return the same rows and labels; batch boundaries
may differ. The trainer checks row and positive-label counts across passes.
Provide separate Dataset instances when independent cursors are needed.

``X`` is a two-dimensional NumPy array or SciPy sparse matrix with
``n_features`` columns and finite values. Column zero corresponds to LibSVM
feature ID 1. ``y`` is a one-dimensional array of binary labels (0 or 1).
Missing sparse values are zero. A fresh Dataset starts at its first row.

``LibSvmFileDataset(path, n_features=16384)`` reads plain or gzip LibSVM files
in batches, skips blank/comment lines, and closes the file at EOF. The next
call after returning ``None`` reopens it from the beginning. ``close()`` and
context-manager cleanup are supported. ``ArrayDataset(X, y)`` adapts matrices
that are already in memory. File paths and ``(X, y)`` pairs passed to ``fit``
are still accepted and wrapped in these adapters.

Custom source, including ClickHouse
----------------------------------

A source can create a fresh query iterator after each completed traversal.
For a database, use a fixed time interval/snapshot and stable ordering so
repeating the query returns the same selection. The following adapter takes
a factory that yields ``(feature_vector, label)`` records; the factory can
open a streamed ClickHouse query using the application's existing client.

.. code-block:: python

  import numpy as np
  from vanga import Dataset, VangaTrainer

  def source_init(self, row_factory, n_features):
    self.row_factory = row_factory
    self.n_features = n_features
    self.iterator = None

  def source_get_rows(self, count):
    if self.iterator is None:
      self.iterator = iter(self.row_factory())
    rows = []
    while len(rows) < count:
      try:
        rows.append(next(self.iterator))
      except StopIteration:
        break
    if not rows:
      self.iterator = None
      return None
    features, labels = zip(*rows)
    return np.asarray(features, dtype=np.float32), np.asarray(labels, dtype=np.int64)

  class QueryDataset(Dataset):
    __init__ = source_init
    get_rows = source_get_rows

  train = QueryDataset(train_row_factory, n_features=16384)
  validation = QueryDataset(validation_row_factory, n_features=16384)
  trainer = VangaTrainer(n_features=16384, num_trees=20, max_depth=4)
  trainer.fit(train, validation, output_dir="query-run")

The application owns custom Dataset resources and should release database
cursors/connections when a traversal ends or training fails. Vanga does not
store a complete dataset in RAM or on disk. The source is read for every
training epoch, validation, final metrics and export; no cache or separate
``reset()`` implementation is required.

Initialization and predictions
------------------------------

The first ``init_rows`` training records (default 1000) initialize the scaler
and gates. These rows also participate in the first training epoch. The scaler
then stays fixed. Subsequent rows are processed incrementally in batches;
shuffling happens within each batch. Memory used for input rows is bounded by
``init_rows`` and the training/evaluation batch sizes, independently of dataset
length. Model activations also depend on the number/depth of trees and branches.

``predict_raw``, ``predict_proba`` and ``predict`` use the best soft checkpoint,
accept Dataset objects, LibSVM paths or feature matrices, and return NumPy
arrays. Dataset/path prediction consumes one traversal. Predicting a matrix
requires no labels; Dataset batches use the same ``(X, y)`` contract.
``predict_proba`` has columns ``[P(y=0), P(y=1)]``. Prediction arrays themselves
are held in memory.

``fit`` also exports hardened trees automatically: ``node_hardened.vanga`` in
Vanga mode, or ``node_hardened_catboost.json`` in ODST modes. Vanga and single
mode exports read raw feature IDs. Projection mode additionally writes
``node_projection_transform.npz``. ``metrics_`` records soft train/validation
logloss, separate hardened-export metrics, initialization settings and peak RSS
(plus GPU memory for CUDA). ``load`` restores the soft checkpoint and scaler
from a completed run.

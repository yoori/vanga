C++ CMake package
=================

Build and install the native library with CMake 3.20+ and a C++20 compiler:

.. code-block:: sh

  cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/foros/vanga -DBUILD_SHARED_LIBS=ON
  cmake --build build/native -j2
  cmake --install build/native

The installation provides headers, libraries and a relocatable CMake package
under ``<libdir>/cmake/Vanga``. Static builds are also supported. Consumers need
only the exported target; it supplies include directories, C++20 and transitive
Gears/Threads dependencies:

.. code-block:: cmake

  find_package(Vanga CONFIG REQUIRED)
  target_link_libraries(my_app PRIVATE Vanga::DTree)

Pass ``-DCMAKE_PREFIX_PATH=/opt/foros/vanga`` when configuring the consumer if
the prefix is outside CMake's normal search paths, or set ``Vanga_DIR`` to the
directory containing ``VangaConfig.cmake``. Public headers use ``<DTree/DTree.hpp>``.
The native ``vanga-devel`` RPM includes this CMake configuration and headers;
the runtime ``vanga`` RPM provides the shared libraries. ``CMAKE_INSTALL_PREFIX``,
``CMAKE_INSTALL_LIBDIR`` and ``CMAKE_INSTALL_INCLUDEDIR`` are honored. The native version is
independent of the Python package version; set it with ``-DVANGA_VERSION=1.0.0.1``.

C++ numeric feature training
----------------------------

``DTreeTrainer train``, ``train-add`` and ``train-trees`` accept numeric LibSVM
features. ``--feature-type=auto`` (default) retains the original binary learner
for 0/1 data and selects the numeric learner for other values or an existing
numeric model. ``--feature-type=float`` forces numeric training, including on
0/1 data. ``--feature-type=binary`` explicitly retains the legacy positive-feature
presence convention; it cannot resume a numeric model.

.. code-block:: sh

  DTreeTrainer train model.vanga train.libsvm validation.libsvm \
    --feature-type=float --steps=100 --depth=6 --border-count=32 \
    --alpha=0.5 --numeric-l2=1 --min-leaf-size=20 --threads=4 \
    --save-best-model=best.vanga
  DTreeTrainer train-add model.vanga train.libsvm validation.libsvm --steps=20

The numeric learner minimizes logloss on the full in-memory training set. It
uses Newton updates with backtracking, adding independent threshold branches
inside native additive DTree containers and updating existing container values.
Different thresholds for the same feature remain separate branches. Thresholds
use midpoints between deterministically sampled training values (up to 4096 finite
observations per feature, plus extremes); histograms account for implicit sparse
zeros and learn which side receives NaNs. Infinite training values are rejected.
Targets may be binary or finite probabilities in [0, 1].

``--depth`` bounds new split depth; ``--steps`` bounds update count;
``--border-count`` is 1..1024, with an additional NaN-separation threshold when
needed. Existing deeper topologies are retained on resume. The first validation
set selects the best checkpoint; the main output remains the last model, and
``--save-best-model`` writes the best model separately. Binary-only options for
bagging, squared-error gain, multi-level lookahead, annealing and negative-gain
search are rejected when explicitly requested in numeric mode.

The C++ API is available in ``<DTree/NumericTreeLearner.hpp>`` through
``Vanga::NumericDataset::load`` and ``Vanga::NumericTreeLearner::fit``. Training
keeps sparse float rows and histogram statistics; it does not expand floats into
binary feature columns. ``print``, ``filter`` and ``ensemble`` also evaluate
numeric models using their original values. Saved ``dtree-float`` models use
original feature IDs and require no quantization file at prediction time.

For C++ prediction use ``FloatFeatureArray`` (sorted by full feature ID) or
``FastFeatureSet``. Its float iterator overload preserves values, including
negative values and NaNs. Full 32-bit IDs are supported without masked-index
collisions; high IDs use a sparse overflow table.

Vanga Python package
====================

The Python package lives in ``python/``. Install it with
``python -m pip install ./python`` (or ``python -m pip install -e ./python``
for development). Python 3.10+ is required. Install the PyTorch build appropriate
for your CUDA driver before installing Vanga if training on a GPU.

Python RPM
----------

``PYTHON=/path/to/python3.12 bash RPM/build-python.sh`` builds the independent
``python3-vanga`` noarch RPM and SRPM. The RPM spec uses ``pyproject-rpm-macros``
to build and install the wheel from a Python-only source archive; it does not
compile the native C++ project or download dependencies. The spec declares
``python3.12-rpm-macros``, ``python3.12-devel`` and ``pyproject-rpm-macros`` as
build requirements, with ``python3.12-setuptools`` and ``python3.12-wheel`` for
the wheel build.
``OUTPUT_DIR`` defaults to ``build/RPMS``. The existing C++ RPM builder is separate.

The RPM installs only Vanga modules/metadata under Python 3.12's standard
``/usr/lib/python3.12/site-packages``, documentation/license and the three CLI
entry points with an explicit Python 3.12 interpreter. Dependencies are ordinary
RPM requirements; no Torch/CUDA, private environment, model weights or C++
predictor are bundled. It provides ``python3.12dist(vanga)`` for consumers.
Application launchers still choose their Torch runtime. Verify that runtime's
actual imports satisfy the dependencies, especially when an isolated vLLM
directory precedes system site-packages.

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

``IndependentVangaTrainer`` runs several adaptive searches sequentially with
seeds ``seed + try_index``, averages their tree functions into one topology,
warm-starts NODE on that topology, then prunes on secondary train while pruning
does not increase its logloss beyond ``prune_epsilon``. The CLI option
``--independent-tries`` selects the number of searches (default 1).

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
may differ. The trainer checks row counts and target sums across passes, allowing
float64 summation tolerance when batch boundaries differ.
Provide separate Dataset instances when independent cursors are needed.

``X`` is a two-dimensional NumPy array or SciPy sparse matrix with
``n_features`` columns and finite values. Column zero corresponds to LibSVM
feature ID 1. ``y`` is a one-dimensional array of finite targets in ``[0, 1]``.
All sources normalize targets to float32 without binarization or clipping. Binary
labels ``0`` and ``1`` use this same contract; no target-mode setting is needed.
Training uses binary cross-entropy with these targets. For the two-logit model,
the scalar logit is ``logit_1 - logit_0``; on binary labels this is mathematically
equivalent to the previous two-class cross-entropy.
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
    return np.asarray(features, dtype=np.float32), np.asarray(labels, dtype=np.float32)

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

Fixed-topology metrics use ``train_target_sum``/``validation_target_sum`` and
``train_mean_target``/``validation_mean_target`` under ``metrics_["rows"]``.
These replace ``train_positives``/``validation_positives``: for binary data the
sum and mean are clicks and CTR, while for probabilities they are expected clicks
and mean target. Saved model structure and prediction APIs are unchanged.

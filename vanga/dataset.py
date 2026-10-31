"""Incremental data sources. Each batch is (X, y); None alone marks end of input.

X is a two-dimensional NumPy or SciPy matrix with zero-based columns. Column 0
corresponds to LibSVM feature ID 1. y contains one binary 0/1 label per row.
get_rows(count) returns 1..count rows, or None. After returning None, the next
call starts the same selection from its beginning. Sources need not implement
length, indexing, iteration or a separate reset method. No full dataset is cached.
"""

from abc import ABC, abstractmethod
import gzip
import io
from numbers import Integral
from os import PathLike
from pathlib import Path

import numpy as np
from scipy import sparse
from sklearn.datasets import load_svmlight_file


def validate_count(count):
  if isinstance(count, bool) or not isinstance(count, Integral) or count <= 0:
    raise ValueError('row count must be a positive integer')
  return int(count)


def normalize_features(values, n_features, allow_empty=False):
  if getattr(values, 'ndim', None) != 2:
    raise ValueError('features must be a two-dimensional NumPy or SciPy matrix')
  if values.shape[1] != n_features:
    raise ValueError('expected {} feature columns, got {}'.format(n_features, values.shape[1]))
  if not allow_empty and not values.shape[0]:
    raise ValueError('empty batch; return None to mark end of dataset')
  if np.issubdtype(values.dtype, np.complexfloating):
    raise ValueError('complex feature values are not supported')
  features = sparse.csr_matrix(values, dtype=np.float32, copy=True)
  features.sum_duplicates()
  features.eliminate_zeros()
  if not np.isfinite(features.data).all():
    raise ValueError('non-finite feature values')
  return features


def validate_rows(rows, n_features, count):
  if not isinstance(rows, tuple) or len(rows) != 2:
    raise ValueError('get_rows must return an (X, y) pair or None')
  features = normalize_features(rows[0], n_features)
  if features.shape[0] > count:
    raise ValueError('get_rows returned more rows than requested')
  labels = np.asarray(rows[1])
  if labels.shape != (features.shape[0],) or not np.isin(labels, (0, 1)).all():
    raise ValueError('expected one binary 0/1 label per row')
  return features, labels.astype(np.int64)


def _get_rows(self, count):
  """Return up to count rows as (X, y), or None; the call after None restarts the selection."""
  raise NotImplementedError


class Dataset(ABC):
  """Implement get_rows(count) to supply raw feature batches to VangaTrainer.fit()."""

  get_rows = abstractmethod(_get_rows)


def _file_initialize(self, path, n_features=16384):
  self.path = Path(path).resolve()
  self.n_features = validate_count(n_features)
  self._file = None
  self._eof_pending = False
  self._stat = None


def _file_close(self):
  stream = getattr(self, '_file', None)
  if stream is not None:
    stream.close()
  self._file = None
  self._eof_pending = False


def _file_get_rows(self, count):
  count = validate_count(count)
  if self._eof_pending:
    self._eof_pending = False
    return None
  if self._file is None:
    self._stat = self.path.stat()
    opener = gzip.open if self.path.suffix == '.gz' else open
    self._file = opener(self.path, 'rb')
  try:
    lines = []
    while len(lines) < count:
      line = self._file.readline()
      if not line:
        after = self.path.stat()
        before = self._stat
        if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
          raise ValueError('input changed while reading: ' + str(self.path))
        self.close()
        self._eof_pending = bool(lines)
        break
      if line.split(b'#', 1)[0].strip():
        lines.append(line)
    if not lines:
      return None
    with io.BytesIO(b''.join(lines)) as source:
      features, labels = load_svmlight_file(
        source, n_features=self.n_features, zero_based=False, dtype=np.float32)
    return validate_rows((features, labels), self.n_features, count)
  except BaseException:
    self.close()
    raise


def _file_enter(self):
  return self


def _file_exit(self, exc_type, exc_value, traceback):
  self.close()


class LibSvmFileDataset(Dataset):
  """Read a one-based LibSVM file (optionally .gz) incrementally; close at EOF.

  get_rows(count) returns a float32 CSR matrix and int64 labels. Blank lines and
  comments are skipped. Supports close() and the with statement.
  get_rows() after None starts a new traversal. No separate reset is needed.
  """

  __init__ = _file_initialize
  get_rows = _file_get_rows
  close = _file_close
  __enter__ = _file_enter
  __exit__ = _file_exit
  __del__ = _file_close


def _array_initialize(self, features, labels):
  if getattr(features, 'ndim', None) != 2:
    raise ValueError('features must be a two-dimensional NumPy or SciPy matrix')
  self._features, self._labels = features, np.asarray(labels)
  self.n_features = features.shape[1]
  self._position = 0
  if self._labels.shape != (features.shape[0],):
    raise ValueError('expected one binary 0/1 label per row')


def _array_get_rows(self, count):
  count = validate_count(count)
  stop = min(self._position + count, self._features.shape[0])
  if self._position == stop:
    self._position = 0
    return None
  indexes = slice(self._position, stop)
  # COO/DOK inputs need CSR indexing; only the explicit in-memory adapter converts them.
  if sparse.issparse(self._features) and self._features.format not in ('csr', 'csc', 'lil'):
    self._features = self._features.tocsr()
  result = self._features[indexes], self._labels[indexes]
  self._position = stop
  return result


class ArrayDataset(Dataset):
  """Supply batches from existing NumPy/SciPy features and binary labels."""

  __init__ = _array_initialize
  get_rows = _array_get_rows


def as_dataset(data, n_features):
  """Return a Dataset and whether the caller owns the resulting adapter."""
  if isinstance(data, Dataset):
    return data, False
  if isinstance(data, (str, PathLike)):
    return LibSvmFileDataset(data, n_features), True
  if isinstance(data, tuple) and len(data) == 2:
    return ArrayDataset(*data), True
  raise ValueError('expected a Dataset, LibSVM path or an (X, y) pair')


def close_dataset(dataset):
  close = getattr(dataset, 'close', None)
  if close is not None:
    close()


def iter_batches(data, count, n_features):
  count = validate_count(count)
  dataset, owned = as_dataset(data, n_features)
  try:
    while True:
      rows = dataset.get_rows(count)
      if rows is None:
        break
      yield validate_rows(rows, n_features, count)
  finally:
    if owned:
      close_dataset(dataset)

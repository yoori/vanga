"""Dataset protocol and genuinely incremental training; run with NodeTrainer dependencies."""

import contextlib
import gzip
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

import numpy as np
from scipy import sparse
import torch

from vanga import ArrayDataset, Dataset, LibSvmFileDataset, VangaTrainer
from vanga.dataset import iter_batches


def generated_init(self, rows, width=4, short=False, observer=None):
  self.rows, self.width, self.short = rows, width, short
  self.position = self.eofs = 0
  self.requests = []
  self.observer = observer


def generated_get_rows(self, count):
  self.requests.append((count, self.position, self.observer() if self.observer else 0))
  if self.position == self.rows:
    self.eofs += 1
    self.position = 0
    return None
  take = min(count, self.rows - self.position, 3 if self.short else count)
  indexes = np.arange(self.position, self.position + take)
  features = np.zeros((take, self.width), dtype=np.float32)
  features[:, 0] = indexes % 2
  features[:, 1] = np.sin(indexes)
  features[:, 2] = indexes % 7 - 3
  features[:, 3] = (indexes % 3 == 0).astype(np.float32)
  self.position += take
  return sparse.csr_matrix(features), (indexes % 2).astype(np.int64)


class GeneratedDataset(Dataset):
  __init__ = generated_init
  get_rows = generated_get_rows


def test_abstract_and_arrays():
  with unittest.TestCase().assertRaises(TypeError):
    Dataset()
  features = np.arange(20, dtype=np.float32).reshape(5, 4)
  labels = np.arange(5) % 2
  source = ArrayDataset(features, labels)
  for _ in range(3):
    a, b = source.get_rows(3), source.get_rows(3)
    np.testing.assert_array_equal(a[0], features[:3])
    np.testing.assert_array_equal(b[1], labels[3:])
    assert source.get_rows(3) is None
  for count in (0, -1, True, 1.5):
    with unittest.TestCase().assertRaises(ValueError):
      source.get_rows(count)


def test_libsvm_restart():
  text = b'# comment\n\n0 1:-2 4:3\n1 2:0.5 # row comment\n0\n1 1:1'
  with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    for suffix in ('.libsvm', '.libsvm.gz'):
      path = root / ('data' + suffix)
      if suffix.endswith('.gz'):
        with gzip.open(path, 'wb') as output:
          output.write(text)
      else:
        path.write_bytes(text)
      with LibSvmFileDataset(path, n_features=4) as source:
        for count in (3, 2, 9, 1):
          chunks = list(iter_batches(source, count, 4))
          values = sparse.vstack([x for x, _ in chunks]).toarray()
          labels = np.concatenate([y for _, y in chunks])
          np.testing.assert_array_equal(labels, [0, 1, 0, 1])
          np.testing.assert_array_equal(values, [[-2, 0, 0, 3], [0, 0.5, 0, 0],
                                                  [0, 0, 0, 0], [1, 0, 0, 0]])
          assert source._file is None
          assert all(0 < len(y) <= count for _, y in chunks)
      assert source._file is None
    for text in ('1.5 1:1\n', '1 5:1\n', '1 1:nan\n'):
      path = root / 'bad.libsvm'
      path.write_text(text)
      with LibSvmFileDataset(path, 4) as source:
        with unittest.TestCase().assertRaises(ValueError):
          source.get_rows(2)
        assert source._file is None


def streaming_training(mode):
  steps = [0]
  original_step = torch.optim.Adam.step

  def counted_step(optimizer, *args, **kwargs):
    result = original_step(optimizer, *args, **kwargs)
    steps[0] += 1
    return result

  with tempfile.TemporaryDirectory() as directory:
    output = Path(directory) / 'run'
    train = GeneratedDataset(53, short=True, observer=lambda: steps[0])
    validation = GeneratedDataset(23, short=True)
    trainer = VangaTrainer(n_features=4, num_trees=2, max_depth=2, feature_mode=mode,
                           epochs=2, patience=0, init_rows=7, batch_size=11,
                           eval_batch_size=13, device='cpu', cpu_threads=1)
    with mock.patch.object(torch.optim.Adam, 'step', counted_step):
      trainer.fit(train, validation, output)
    assert trainer.metrics_['status'] == 'complete'
    assert trainer.metrics_['rows'] == {'train': 53, 'validation': 23,
                                       'train_target_sum': 26.0, 'validation_target_sum': 11.0,
                                       'train_mean_target': 26 / 53,
                                       'validation_mean_target': 11 / 23}
    assert trainer.metrics_['preprocessing']['scaler_fit_rows'] == 7
    assert trainer.metrics_['preprocessing']['dataset_cache'] is False
    # Native Vanga export reads model parameters only; ODST export also scores data.
    assert train.eofs == validation.eofs == (3 if mode == 'vanga' else 4)
    assert train.position == validation.position == 0
    assert max(count for count, _, _ in train.requests) <= 13
    # Only the initialization prefix is fetched before the first optimizer update.
    first_update_read = next(position for _, position, updates in train.requests if updates > 0)
    assert first_update_read == 7
    expected = {'metrics.json', 'history.jsonl', 'feature_scaler.joblib', 'node_checkpoint.pt',
                 'export_metrics.json'}
    if mode == 'vanga':
      expected.add('node_hardened.vanga')
    else:
      expected.add('node_hardened_catboost.json')
      if mode == 'projection':
        expected.add('node_projection_transform.npz')
    assert {path.name for path in output.iterdir()} == expected
    # Different requested batch sizes and short source batches preserve every row.
    predictions = trainer.predict_proba(validation, batch_size=5)
    assert predictions.shape == (23, 2) and validation.position == 0
    np.testing.assert_allclose(predictions.sum(axis=1), 1)
    restored = VangaTrainer.load(output)
    np.testing.assert_allclose(restored.predict_proba(validation, batch_size=5), predictions,
                               atol=0, rtol=0)
    assert json.loads((output / 'metrics.json').read_text()) == trainer.metrics_


def test_stream_vanga():
  streaming_training('vanga')


def test_stream_single():
  streaming_training('single')


def test_stream_projection():
  streaming_training('projection')


def broken_get_rows(self, count):
  if getattr(self, 'done', False):
    return None
  self.done = True
  return np.zeros((min(count, 2), 4), dtype=np.float32), np.zeros(min(count, 2), dtype=np.int64)


class BrokenDataset(Dataset):
  get_rows = broken_get_rows


def test_restart_required():
  with tempfile.TemporaryDirectory() as directory:
    output = Path(directory) / 'run'
    trainer = VangaTrainer(n_features=4, num_trees=1, max_depth=1, init_rows=2,
                           epochs=2, patience=0, device='cpu', cpu_threads=1)
    with unittest.TestCase().assertRaisesRegex(ValueError, 'did not restart'):
      trainer.fit(BrokenDataset(), GeneratedDataset(11), output)
    assert trainer.metrics_['status'] == 'failed'
    assert not trainer.model_


if __name__ == '__main__':
  tests = [test_abstract_and_arrays, test_libsvm_restart, test_stream_vanga,
           test_stream_single, test_stream_projection, test_restart_required]
  with contextlib.redirect_stdout(io.StringIO()):
    suite = unittest.TestSuite(unittest.FunctionTestCase(test) for test in tests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
  raise SystemExit(0 if result.wasSuccessful() else 1)

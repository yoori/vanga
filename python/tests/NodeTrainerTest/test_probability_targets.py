"""Float-target loading, loss parity, training and hardened export on CPU."""

import gzip
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

import numpy as np
from scipy import sparse
from sklearn.datasets import dump_svmlight_file
import torch
import torch.nn.functional as functional

from vanga import AdaptiveVangaTrainer, ArrayDataset, Dataset, LibSvmFileDataset, NodeTrainer
from vanga import VangaTrainer
from vanga.dataset import iter_batches, normalize_targets
from vanga.utils.NodeTrainer import trainer
from vanga.utils.NodeTrainer.adaptive_export import _hard_batch, evaluate_hard_vanga
from vanga.utils.NodeTrainer.adaptive_model import AdaptiveVangaModel
from vanga.utils.NodeTrainer.multibranch import branch_selectors, hard_vanga_raw


def data():
  rng = np.random.default_rng(19)
  features = rng.normal(size=(96, 4)).astype(np.float32)
  targets = (0.0042 + 0.8 / (1 + np.exp(-features[:, 0]))).astype(np.float32)
  return features, targets


class ChangingBatches(Dataset):
  def __init__(self, features, targets):
    self.features, self.targets = features, targets
    self.position = self.pass_number = 0

  def get_rows(self, count):
    if self.position == len(self.targets):
      self.position = 0
      self.pass_number += 1
      return None
    end = min(self.position + min(count, 1 + self.pass_number % 5), len(self.targets))
    result = self.features[self.position:end], self.targets[self.position:end]
    self.position = end
    return result


class ProbabilityTargetsTest(unittest.TestCase):
  @classmethod
  def setUpClass(cls):
    torch.set_num_threads(1)

  def test_array_and_file_loaders_preserve_float_targets(self):
    values = np.array([0, 0.0042, 0.25, 1], dtype=np.float64)
    features = np.eye(4, dtype=np.float32)
    source = ArrayDataset(features, values)
    for _ in range(2):
      chunks = list(iter_batches(source, 3, 4))
      actual = np.concatenate([y for _, y in chunks])
      self.assertEqual(actual.dtype, np.float32)
      np.testing.assert_array_equal(actual, values.astype(np.float32))
    with tempfile.TemporaryDirectory() as directory:
      for suffix in ('.libsvm', '.libsvm.gz'):
        path = Path(directory) / ('probabilities' + suffix)
        text = '0 1:1\n0.0042 2:1\n0.25 3:1\n1 4:1\n'
        if suffix.endswith('.gz'):
          with gzip.open(path, 'wt') as output:
            output.write(text)
        else:
          path.write_text(text)
        with LibSvmFileDataset(path, 4) as source:
          for count in (1, 3, 8):
            actual = np.concatenate([y for _, y in iter_batches(source, count, 4)])
            np.testing.assert_array_equal(actual, values.astype(np.float32))
        _, actual, _ = trainer.load_training_data(path, 4)
        np.testing.assert_array_equal(actual, values.astype(np.float32))
      _, actual, _ = trainer.load_training_data((features, values), 4)
      np.testing.assert_array_equal(actual, values.astype(np.float32))

  def test_invalid_targets_are_rejected_before_float32_conversion(self):
    for values in ([float('nan')], [float('inf')], [-float('inf')], [-1e-12],
                   [1 + 1e-12], [None], ['0.5'], [0.5 + 0j], [[0.5]], []):
      with self.subTest(values=values), self.assertRaises(ValueError):
        normalize_targets(values, 1)
    np.testing.assert_array_equal(normalize_targets([False, True], 2), [0., 1.])
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / 'invalid.libsvm'
      for value in ('nan', 'inf', '-0.1', '1.1'):
        path.write_text(value + ' 1:1\n')
        with LibSvmFileDataset(path, 4) as source:
          with self.assertRaises(ValueError):
            source.get_rows(1)
          self.assertIsNone(source._file)

  def test_binary_cross_entropy_matches_previous_loss_and_gradients(self):
    torch.manual_seed(5)
    original = torch.randn(37, 2, dtype=torch.float64, requires_grad=True)
    updated = original.detach().clone().requires_grad_()
    targets = torch.arange(37) % 2
    expected = functional.cross_entropy(original, targets)
    actual = trainer.binary_loss(updated, targets.float())
    expected.backward()
    actual.backward()
    torch.testing.assert_close(actual, expected, rtol=1e-14, atol=1e-14)
    torch.testing.assert_close(updated.grad, original.grad, rtol=1e-14, atol=1e-14)
    # Tiny probabilities must survive: dL/dz = sigmoid(z) - target.
    output = torch.zeros(3, 2, dtype=torch.float64, requires_grad=True)
    probabilities = np.array([0.0042, 0.25, 0.9999])
    trainer.binary_loss(output, probabilities, reduction='sum').backward()
    np.testing.assert_allclose(output.grad[:, 1], 0.5 - probabilities, atol=1e-15)
    np.testing.assert_allclose(output.grad[:, 0], probabilities - 0.5, atol=1e-15)

  def test_target_statistics_are_independent_of_batch_boundaries(self):
    features = np.zeros((53, 4), dtype=np.float32)
    targets = np.linspace(0.0001, 0.0099, 53, dtype=np.float32)
    source = ChangingBatches(features, targets)
    model = trainer.make_model(4, 2, 2, 'vanga', 2)
    reference = None
    for count in (3, 7, 11):
      stats = trainer.evaluate_dataset(model, source, count, torch.device('cpu'),
                                       'dense', np.ones(4, dtype=np.float32))
      trainer.check_pass(stats, reference, 'validation')
      self.assertAlmostEqual(stats['target_sum'], targets.sum(dtype=np.float64), places=14)
      self.assertAlmostEqual(stats['mean_target'], targets.mean(dtype=np.float64), places=14)
      reference = stats
    # Previously both sums would truncate to zero.
    with self.assertRaisesRegex(ValueError, 'Dataset changed'):
      trainer.check_pass({'rows': 53, 'target_sum': reference['target_sum'] + .01},
                         reference, 'validation')
    trainer.check_pass({'rows': 53, 'target_sum': reference['target_sum'] + 1e-14},
                       reference, 'validation')

  def test_training_reload_and_hardened_export_with_probabilities(self):
    features, targets = data()
    for mode in ('projection', 'single', 'vanga'):
      with self.subTest(mode=mode), tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / 'run'
        model = VangaTrainer(n_features=4, num_trees=2, max_depth=2, feature_mode=mode,
          init_rows=17, epochs=2, patience=0, batch_size=19, eval_batch_size=23,
          device='cpu', cpu_threads=1)
        model.fit(ChangingBatches(features, targets), ArrayDataset(features, targets), output)
        self.assertEqual(model.metrics_['status'], 'complete')
        self.assertEqual(model.metrics_['rows']['train_target_sum'], targets.sum(dtype=np.float64))
        raw = model.predict_raw(features)
        expected = float(np.mean(np.logaddexp(0, raw) - targets * raw))
        self.assertAlmostEqual(model.metrics_['validation_logloss'], expected, places=6)
        restored = VangaTrainer.load(output)
        np.testing.assert_array_equal(raw, restored.predict_raw(features))
        if mode != 'vanga':
          from catboost import CatBoostClassifier
          native = CatBoostClassifier()
          native.load_model(model.metrics_['export']['json_model'], format='json')
          prediction_input = features
          if mode == 'projection':
            with np.load(model.metrics_['export']['projection_transform']) as transform:
              prediction_input = ((features * (1 / model.scaler_.scale_)) @ transform['selectors'])
          hard = native.predict(prediction_input, prediction_type='RawFormulaVal')
          expected = float(np.mean(np.logaddexp(0, hard) - targets * hard))
          self.assertAlmostEqual(model.metrics_['export']['datasets']['validation']['logloss'],
                                 expected, places=12)
        binary = os.environ.get('VANGA_TEST_BINARY')
        if mode == 'vanga' and binary:
          inputs = model.model_
          with torch.no_grad():
            columns = branch_selectors(inputs.feature_selection_logits).argmax(0).numpy()
          selected = features[:, columns.reshape(-1)].reshape((-1,) + columns.shape)
          scaled = selected * (1 / model.scaler_.scale_)[columns]
          bits = scaled > inputs.feature_thresholds.detach().numpy()
          reference = hard_vanga_raw(bits, inputs.response.detach().numpy() / inputs.num_trees,
                                     float(inputs.bias.detach()))
          path = Path(directory) / 'predict.libsvm'
          dump_svmlight_file(features, np.zeros(len(features)), str(path), zero_based=False)
          result = subprocess.run([binary, 'predict-raw',
            model.metrics_['export']['native_model'], str(path)],
            text=True, capture_output=True, check=True)
          np.testing.assert_allclose(np.fromstring(result.stdout, sep='\n'), reference,
                                     rtol=0, atol=1e-12)

  def test_adaptive_node_training_pruning_and_export_accept_probabilities(self):
    features, targets = data()
    source = lambda: ArrayDataset(features, targets)
    device = torch.device('cpu')
    model = AdaptiveVangaModel(4)
    node = NodeTrainer(epochs=2, patience=2, batch_size=19, learning_rate=.05)
    result = node.fit(model, source(), source(), np.ones(4, dtype=np.float32), device)
    self.assertLess(result['best_secondary_soft_logloss'], math.log(2))
    hard = evaluate_hard_vanga(model, source(), np.ones(4), 17, 4)
    self.assertAlmostEqual(hard['logloss'], result['best_secondary_soft_logloss'], places=6)
    with tempfile.TemporaryDirectory() as directory:
      output = Path(directory) / 'adaptive'
      adaptive = AdaptiveVangaTrainer(n_features=4, mutation_nodes=1, extend_extra_nodes=1,
        epochs=1, node_patience=1, topology_patience=1, min_improvement=.5,
        batch_size=19, init_rows=17, device='cpu', cpu_threads=1, verbose=False)
      adaptive.fit(source(), source(), source(), output)
      self.assertEqual(adaptive.metrics_['status'], 'complete')
      raw, _ = _hard_batch(adaptive.model_, sparse.csr_matrix(features), adaptive.scaler_.scale_)
      expected = float(np.mean(np.logaddexp(0, raw) - targets * raw))
      self.assertAlmostEqual(adaptive.metrics_['best_validation_logloss'], expected, places=12)
      self.assertTrue((output / 'best_vanga_model.vanga').is_file())
      metrics = json.loads((output / 'metrics.json').read_text())
      self.assertTrue(all(step['pruning']['requested_nodes'] == 1 for step in metrics['history']))


if __name__ == '__main__':
  unittest.main()

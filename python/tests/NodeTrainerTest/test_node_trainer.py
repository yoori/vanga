"""Run with NodeTrainer dependencies installed and --binary pointing to DTreeTrainer."""

import argparse
import contextlib
import copy
import gzip
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[2] / 'vanga' / 'utils' / 'NodeTrainer'
sys.path.insert(0, str(SOURCE.parents[2]))

import joblib
import numpy as np
from scipy import sparse
from sklearn.datasets import dump_svmlight_file
import torch
import torch.nn.functional as functional
from catboost import CatBoostClassifier

from vanga.utils.NodeTrainer import VangaTrainer, trainer
from vanga.utils.NodeTrainer import train as cli
from vanga.utils.NodeTrainer.export import raw_float32_borders
from vanga.utils.NodeTrainer.multibranch import branch_selectors, hard_vanga_raw, initialize_vanga
from vanga.utils.NodeTrainer.vanga_export import export_vanga


def dataset(rows=1024):
  rng = np.random.default_rng(12)
  values = rng.normal(size=(rows, 13)).astype(np.float32)
  values[rng.random(values.shape) < 0.7] = 0
  labels = ((values[:, 0] + values[:, 1] + rng.normal(size=rows) * 0.5) > 0).astype(np.int64)
  return sparse.csr_matrix(values), labels


def test_labels_and_cli():
  with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    data = root / 'data.libsvm'
    data.write_text('1.5 1:1\n1 2:3\n')
    with unittest.TestCase().assertRaisesRegex(ValueError, r'\[0, 1\]'):
      trainer.load_libsvm(data, 13)
    data.write_text('0 1:1\n1 2:3\n')
    base = ['--train', str(data), '--validation', str(data), '--output-dir', str(root / 'run')]
    args = cli.parse_args(base + ['--num-trees', '7', '--depth', '3'])
    assert args.num_trees == 7 and args.max_depth == 3 and args.device == 'cuda'
    assert args.feature_mode == 'projection'
    assert cli.parse_args(base + ['--feature-mode', 'single']).feature_mode == 'single'
    args = cli.parse_args(base + ['--feature-mode', 'vanga'])
    assert args.features_per_node == 2
    args = cli.parse_args(base + ['--feature-mode', 'vanga', '--features-per-node', '3',
                                  '--num-trees', '2', '--max-depth', '2'])
    assert args.features_per_node == 3
    invalid = (['--num-trees', '0'], ['--max-depth', '17'], ['--learning-rate', 'nan'],
               ['--features-per-node', '2'],
               ['--feature-mode', 'vanga', '--features-per-node', '0'],
               ['--feature-mode', 'vanga', '--features-per-node', '3', '--n-features', '2'],
               ['--feature-mode', 'vanga', '--max-depth', '16'])
    for option in invalid:
      with contextlib.redirect_stderr(io.StringIO()):
        with unittest.TestCase().assertRaises(SystemExit):
          cli.parse_args(base + option)
    if not torch.cuda.is_available():
      with unittest.TestCase().assertRaisesRegex(RuntimeError, 'CUDA requested but unavailable'):
        trainer.choose_device('cuda')


def test_sparse_dense_gradients():
  torch.set_num_threads(1)
  torch.manual_seed(4)
  values, labels = dataset()
  for feature_mode in ('projection', 'single', 'vanga'):
    model = trainer.make_model(13, 3, 2, feature_mode).to(DEVICE)
    with torch.no_grad():
      inputs = trainer.make_batch(values, slice(None), DEVICE, 'dense')
      if feature_mode == 'vanga':
        initialize_vanga(model, inputs, labels)
      else:
        model(inputs)
    other = copy.deepcopy(model)
    dense = trainer.logits(model, trainer.make_batch(values, slice(0, 64), DEVICE, 'dense'))
    csr = trainer.logits(other, trainer.make_batch(values, slice(0, 64), DEVICE, 'sparse'))
    torch.testing.assert_close(dense, csr, atol=1e-6, rtol=1e-5)
    targets = torch.as_tensor(labels[:64], dtype=torch.long, device=DEVICE)
    functional.cross_entropy(dense, targets).backward()
    functional.cross_entropy(csr, targets).backward()
    gradients = {name: value.grad for name, value in model.named_parameters()}
    for name, value in other.named_parameters():
      if value.requires_grad:
        assert value.grad is not None and gradients[name] is not None, name
        torch.testing.assert_close(value.grad, gradients[name], atol=1e-6, rtol=1e-4)
    if feature_mode in ('single', 'vanga'):
      if feature_mode == 'single':
        selectors = model.choice_function(model.feature_selection_logits, dim=0)
      else:
        selectors = branch_selectors(model.feature_selection_logits)
      assert torch.all((selectors == 0) | (selectors == 1))
      assert torch.all(selectors.sum(dim=0) == 1)
      chosen = selectors.argmax(dim=0).cpu().numpy()
      if feature_mode == 'vanga':
        assert np.all(chosen[..., 0] != chosen[..., 1])
      observed = values[:64, chosen.reshape(-1)].toarray().reshape((64,) + chosen.shape)
      selected = values[:64] @ selectors.detach().cpu().numpy().reshape(13, -1)
      np.testing.assert_array_equal(selected.reshape(observed.shape), observed)
      gradient = model.feature_selection_logits.grad
      assert torch.isfinite(gradient).all() and torch.count_nonzero(gradient) > 0
      before = model.feature_selection_logits.detach().clone()
      torch.optim.SGD(model.parameters(), lr=1e-4).step()
      assert not torch.equal(before, model.feature_selection_logits)
      raw = values.copy()
      trainer.make_batch(values, slice(None), DEVICE, 'sparse', np.full(13, 2, np.float32))
      np.testing.assert_array_equal(raw.data, values.data)


def test_multibranch_additive_semantics():
  model = trainer.make_model(2, 2, 2, 'vanga', 2).to(DEVICE)
  with torch.no_grad():
    model.feature_selection_logits.fill_(-10)
    model.feature_selection_logits[0, :, :, 0] = 10
    model.feature_selection_logits[1, :, :, 1] = 10
    model.feature_thresholds.zero_()
    model.log_temperatures.fill_(-30)
    model.bias.fill_(0.25)
    model.response.copy_(torch.arange(32, device=DEVICE).reshape(2, 16) * 0.1)
  inputs = torch.tensor([[-1, -1], [-1, 1], [1, -1], [1, 1]], device=DEVICE).float()
  response = model.response.detach().cpu().numpy().astype(np.float64)
  expected = []
  for row in inputs.cpu().numpy():
    raw = 0.25
    for tree in range(2):
      for first in range(2):
        child = 2 * first + int(row[first] > 0)
        for second in range(2):
          leaf = 4 * child + 2 * second + int(row[second] > 0)
          raw += response[tree, leaf] / 2
    expected.append(raw)
  logits = trainer.logits(model, inputs).detach().cpu().numpy()
  np.testing.assert_allclose(logits[:, 1] - logits[:, 0], expected, atol=1e-6, rtol=1e-6)
  # All candidates tie: selecting every feature must still give distinct IDs and finite gradients.
  scores = torch.zeros(3, 2, 2, 3, requires_grad=True, device=DEVICE)
  selectors = branch_selectors(scores)
  chosen = selectors.argmax(dim=0).cpu().numpy()
  np.testing.assert_array_equal(np.sort(chosen, axis=-1), np.broadcast_to([0, 1, 2], (2, 2, 3)))
  (selectors * torch.arange(3, device=DEVICE)[:, None, None, None]).sum().backward()
  assert torch.isfinite(scores.grad).all()


def verify_vanga_predictions(model, scale, features, labels, native, root):
  with torch.no_grad():
    columns = branch_selectors(model.feature_selection_logits).argmax(dim=0).cpu().numpy()
  thresholds = model.feature_thresholds.detach().cpu().numpy()
  values = features[:, columns.reshape(-1)].toarray().reshape((-1,) + columns.shape)
  values *= (1.0 / scale)[columns]
  bits = values > thresholds
  responses = model.response.detach().cpu().numpy().astype(np.float64) / model.num_trees
  expected = hard_vanga_raw(bits, responses, float(model.bias.detach().cpu()))
  data = root / 'native-validation.libsvm'
  dump_svmlight_file(features, labels, str(data), zero_based=False)
  prediction = subprocess.run([str(BINARY), 'predict-raw', str(native), str(data)],
                              text=True, capture_output=True, check=True)
  actual = np.fromstring(prediction.stdout, sep='\n')
  np.testing.assert_allclose(actual, expected, rtol=0, atol=1e-12)
  margins = np.min(np.abs(values - thresholds), axis=tuple(range(1, values.ndim)))
  away = np.flatnonzero(margins > 1e-4)
  assert len(away)
  hardened = copy.deepcopy(model)
  with torch.no_grad():
    hardened.log_temperatures.fill_(-30)
    inputs = trainer.make_batch(features, away, DEVICE, 'sparse', 1.0 / scale)
    raw = trainer.logits(hardened, inputs).cpu().numpy()
  np.testing.assert_allclose(raw[:, 1] - raw[:, 0], expected[away], atol=1e-6, rtol=1e-6)
  lines = native.read_text().splitlines()
  assert lines[0] == 'dtree-float'
  ids = [int(line.split('\t')[0]) for line in lines[1:]]
  assert len(ids) == len(set(ids))
  width = 2 * model.features_per_node
  count = 1 + model.num_trees * ((width ** (model.depth + 1) - 1) // (width - 1) - 1)
  assert len(ids) == count
  for index, line in enumerate(lines[1:]):
    checks = line.split('\t')[2]
    if checks:
      factor = model.num_trees if index == 0 else 1
      assert len(checks.split('|')) == factor * model.features_per_node
  return expected


def test_multibranch_native_export():
  torch.manual_seed(19)
  # Cover different branch counts/depths, sparse zeros, repeated level checks and merged roots.
  for branches, depth in ((1, 1), (2, 1), (2, 3), (3, 2)):
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory)
      model = trainer.make_model(13, 2, depth, 'vanga', branches).to(DEVICE)
      features, labels = dataset(64)
      scale = np.geomspace(0.1, 10, 13).astype(np.float32)
      inputs = trainer.make_batch(features, slice(None), DEVICE, 'dense', 1.0 / scale)
      initialize_vanga(model, inputs, labels)
      columns = branch_selectors(model.feature_selection_logits).argmax(dim=0).cpu().numpy()
      thresholds = model.feature_thresholds.detach().cpu().numpy()
      borders = raw_float32_borders(thresholds, (1.0 / scale)[columns])
      probes = [np.random.default_rng(7).normal(size=(16, 13)).astype(np.float32)]
      for column, border in zip(columns.reshape(-1), borders.reshape(-1)):
        neighbors = [np.nextafter(border, np.float32(-np.inf)), border,
                     np.nextafter(border, np.float32(np.inf))]
        rows = np.zeros((3, 13), dtype=np.float32)
        rows[:, column] = neighbors
        probes.append(rows)
      probes = np.concatenate(probes)
      features = sparse.vstack((features, sparse.csr_matrix(probes)), format='csr')
      labels = np.concatenate((labels, np.zeros(len(probes), dtype=np.int64)))
      result = export_vanga(model, scale, root)
      verify_vanga_predictions(model, scale, features, labels,
                               root / 'node_hardened.vanga', root)
      assert result['feature_mode'] == 'vanga' and result['features_per_node'] == branches
      assert result['input_space'] == 'raw_libsvm' and result['projection_transform'] is None
      assert not list(root.glob('*.npz'))


def test_raw_border_rounding():
  borders = np.array([-3, -1, 0, 0, 1, 3, 4], dtype=np.float32)
  inverse = np.array([1e-38, 10, 1e-30, 1e30, 0.1, 2.3, 1e-38], dtype=np.float32)
  folded = raw_float32_borders(borders, inverse)
  with np.errstate(over='ignore', under='ignore'):
    neighbors = np.stack([np.nextafter(folded, np.float32(-np.inf)), folded,
                          np.nextafter(folded, np.float32(np.inf)),
                          np.zeros_like(folded), np.full_like(folded, -1)], axis=0)
    np.testing.assert_array_equal(neighbors * inverse > borders, neighbors > folded)
  assert folded[2] > 0, 'rounding plateau at zero must be retained'
  with unittest.TestCase().assertRaisesRegex(ValueError, 'finite raw float32 boundary'):
    raw_float32_borders(np.array([-4], np.float32), np.array([1e-38], np.float32))


def training_export_and_vanga(feature_mode):
  with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    train_x, train_y = dataset()
    val_x, val_y = dataset(256)
    paths = []
    for name, x, y in (('train', train_x, train_y), ('validation', val_x, val_y)):
      path = root / name / 'data.libsvm.gz'
      path.parent.mkdir()
      with gzip.open(path, 'wb') as target:
        dump_svmlight_file(x, y, target, zero_based=False)
      paths.append(path)
    output = root / 'run'
    command = [sys.executable, str(SOURCE / 'train.py'), '--train', str(paths[0]),
               '--validation', str(paths[1]), '--output-dir', str(output), '--n-features', '13',
               '--num-trees', '3', '--max-depth', '2', '--epochs', '2', '--batch-size', '64',
               '--eval-batch-size', '64', '--cpu-threads', '1', '--device', str(DEVICE),
               '--feature-mode', feature_mode]
    if feature_mode == 'vanga':
      command += ['--features-per-node', '2', '--epochs', '5', '--learning-rate', '0.03']
    completed = subprocess.run(command, text=True, capture_output=True, timeout=180)
    assert completed.returncode == 0, completed.stdout + completed.stderr
    result = json.loads((output / 'metrics.json').read_text())
    assert result['status'] == 'complete'
    assert result['device'] == str(DEVICE)
    assert result['model']['num_trees'] == 3 and result['model']['depth'] == 2
    assert result['model']['feature_mode'] == feature_mode
    assert result['rows']['train'] == 1024 and result['rows']['validation'] == 256
    assert result['best_epoch'] in range(1, 6 if feature_mode == 'vanga' else 3)
    assert abs(result['validation_logloss'] - result['best_validation_logloss']) < 1e-7
    checkpoint = torch.load(output / 'node_checkpoint.pt', map_location='cpu', weights_only=True)
    assert checkpoint['model'] == result['model']
    model = trainer.make_model(13, 3, 2, feature_mode).to(DEVICE)
    restored = {k.removeprefix('trees.'): v for k, v in checkpoint['state_dict'].items()}
    model.load_state_dict(restored)
    scale = joblib.load(output / 'feature_scaler.joblib').scale_
    scaled = val_x.multiply(1.0 / scale).tocsr()
    soft_loss = trainer.evaluate(model, scaled, val_y, 64, DEVICE, 'sparse')
    assert abs(soft_loss - result['validation_logloss']) < 1e-7
    api = VangaTrainer.load(output, device=str(DEVICE))
    api_raw = api.predict_raw(val_x)
    api_loss = np.mean(np.logaddexp(0, api_raw) - val_y * api_raw)
    assert abs(api_loss - result['validation_logloss']) < 1e-6
    if feature_mode == 'vanga':
      assert result['model']['features_per_node'] == 2
      assert result['train_logloss'] < 0.69
      verify_vanga_predictions(model, scale, val_x, val_y,
                               output / 'node_hardened.vanga', root)
      assert not (output / 'node_projection_transform.npz').exists()
      old_metrics = (output / 'metrics.json').read_bytes()
      repeated = subprocess.run(command, text=True, capture_output=True, timeout=30)
      assert repeated.returncode != 0 and 'already exists' in repeated.stderr
      assert (output / 'metrics.json').read_bytes() == old_metrics
      return
    if feature_mode == 'projection':
      transform = np.load(output / 'node_projection_transform.npz')
      assert transform['selectors'].shape == (13, 6)
      np.testing.assert_array_equal(scale, transform['scaler_scale'])
      projected = (scaled @ sparse.csr_matrix(transform['selectors'])).toarray()
      prediction_input = projected
    else:
      assert not (output / 'node_projection_transform.npz').exists()
      assert result['export']['projection_transform'] is None
      assert result['export']['input_space'] == 'raw_libsvm'
      chosen = model.feature_selection_logits.argmax(dim=0).cpu().numpy()
      assert result['export']['selected_feature_ids'] == (chosen + 1).tolist()
      projected = scaled[:, chosen.reshape(-1)].toarray()
      prediction_input = val_x
    catboost = CatBoostClassifier()
    catboost.load_model(str(output / 'node_hardened_catboost.json'), format='json')
    assert catboost.tree_count_ == 3
    raw = catboost.predict(prediction_input, prediction_type='RawFormulaVal')
    raw = np.asarray(raw).reshape(-1)
    loss = float(np.mean(np.logaddexp(0, raw) - val_y * raw))
    assert abs(loss - result['export']['datasets']['validation']['logloss']) < 1e-12
    # Verify hardening against ODST itself, independently of exported leaf-index arithmetic.
    margins = projected.reshape(-1, 3, 2) - model.feature_thresholds.detach().cpu().numpy()
    away_from_borders = np.min(np.abs(margins), axis=(1, 2)) > 1e-4
    assert away_from_borders.any()
    with torch.no_grad():
      model.log_temperatures.fill_(-30)
      inputs = trainer.make_batch(scaled, np.flatnonzero(away_from_borders), DEVICE, 'dense')
      hardened = trainer.logits(model, inputs).cpu().numpy()
    np.testing.assert_allclose(hardened[:, 1] - hardened[:, 0], raw[away_from_borders],
                               atol=1e-6, rtol=1e-6)
    model_json = json.loads((output / 'node_hardened_catboost.json').read_text())
    assert all(len(tree['splits']) == 2 for tree in model_json['oblivious_trees'])
    if feature_mode == 'single':
      used = {f['flat_feature_index'] + 1 for f in model_json['features_info']['float_features']}
      assert used == set((chosen + 1).reshape(-1).tolist())
    native = root / 'model.vanga'
    subprocess.run([str(BINARY), 'import-catboost',
                    str(output / 'node_hardened_catboost.json'), str(native)], check=True)
    projected_file = root / 'validation.libsvm'
    dump_svmlight_file(prediction_input, val_y, str(projected_file), zero_based=False)
    prediction = subprocess.run([str(BINARY), 'predict-raw', str(native), str(projected_file)],
                                text=True, capture_output=True, check=True)
    actual = np.fromstring(prediction.stdout, sep='\n')
    np.testing.assert_allclose(actual, raw, rtol=0, atol=1e-12)
    old_metrics = (output / 'metrics.json').read_bytes()
    repeated = subprocess.run(command, text=True, capture_output=True, timeout=30)
    assert repeated.returncode != 0 and 'already exists' in repeated.stderr
    assert (output / 'metrics.json').read_bytes() == old_metrics


def test_projection_training_export():
  training_export_and_vanga('projection')


def test_single_feature_training_export():
  training_export_and_vanga('single')


def test_multibranch_training_export():
  training_export_and_vanga('vanga')


def test_python_api_validation():
  invalid = ({'num_trees': 0}, {'epochs': 0}, {'patience': -1}, {'seed': 1.5},
             {'batch_size': True}, {'learning_rate': float('nan')}, {'max_depth': 17},
             {'features_per_node': 14, 'n_features': 13}, {'max_depth': 16},
             {'feature_mode': 'invalid'}, {'input_mode': 'invalid'}, {'eval_batch_size': 0},
             {'device': None})
  for settings in invalid:
    with unittest.TestCase().assertRaises(ValueError):
      VangaTrainer(**settings)
  model = VangaTrainer(n_features=13, device=str(DEVICE))
  with unittest.TestCase().assertRaisesRegex(RuntimeError, 'not fitted'):
    model.predict_proba(np.zeros((1, 13), dtype=np.float32))
  with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    for features, labels in ((np.zeros((1, 12)), [1]), (np.zeros((1, 13)), [1.5]),
                              (np.zeros((2, 13)), [0]), (np.full((1, 13), np.nan), [1])):
      output = root / str(len(list(root.iterdir())))
      with unittest.TestCase().assertRaises(ValueError):
        model.fit((features, labels), (features, labels), output)
      assert model.model_ is None
      assert model.metrics_['status'] == 'failed'
      with unittest.TestCase().assertRaisesRegex(ValueError, 'completed training'):
        VangaTrainer.load(output)


def python_api_training(feature_mode):
  with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    train_x, train_y = dataset(256)
    val_x, val_y = dataset(96)
    original = train_x.copy()
    settings = dict(n_features=13, num_trees=3, max_depth=2, epochs=2, batch_size=64,
                    cpu_threads=1, device=str(DEVICE), feature_mode=feature_mode)
    model = VangaTrainer(**settings)
    captured = io.StringIO()
    with contextlib.redirect_stdout(captured):
      fitted = model.fit((train_x, train_y), (val_x.toarray(), val_y), root / 'run')
    assert fitted is model and not captured.getvalue()
    assert model.metrics_['status'] == 'complete'
    assert model.model_ is not None and model.scaler_ is not None
    np.testing.assert_array_equal(train_x.data, original.data)
    np.testing.assert_array_equal(train_x.indices, original.indices)
    raw = model.predict_raw(val_x)
    dense_raw = model.predict_raw(val_x.toarray(), batch_size=23)
    np.testing.assert_allclose(dense_raw, raw, rtol=1e-5, atol=1e-6)
    np.testing.assert_allclose(model.predict_raw(val_x.todok()), raw, rtol=0, atol=0)
    probabilities = model.predict_proba(val_x)
    assert probabilities.shape == (96, 2) and np.isfinite(probabilities).all()
    np.testing.assert_allclose(probabilities.sum(axis=1), 1, rtol=0, atol=1e-15)
    np.testing.assert_array_equal(model.predict(val_x), probabilities.argmax(axis=1))
    loss = np.mean(np.logaddexp(0, raw) - val_y * raw)
    assert abs(loss - model.metrics_['validation_logloss']) < 1e-6
    assert model.predict_raw(sparse.csr_matrix((0, 13))).shape == (0,)
    assert model.predict_proba(np.empty((0, 13))).shape == (0, 2)
    with unittest.TestCase().assertRaises(ValueError):
      model.predict_raw(np.zeros((2, 12)))
    with unittest.TestCase().assertRaises(ValueError):
      model.predict_raw(val_x, batch_size=0)
    restored = VangaTrainer.load(root / 'run', device=str(DEVICE))
    np.testing.assert_allclose(restored.predict_raw(val_x), raw, rtol=0, atol=0)
    assert restored.metrics_ == model.metrics_
    data = root / 'validation.libsvm.gz'
    with gzip.open(data, 'wb') as target:
      dump_svmlight_file(val_x, val_y, target, zero_based=False)
    np.testing.assert_allclose(model.predict_raw(data), raw, rtol=0, atol=0)
    # An independent caller imports the package from outside the repository and uses the checkpoint.
    script = ('from vanga.utils.NodeTrainer import VangaTrainer; '
              'import sys; m = VangaTrainer.load(sys.argv[1]); '
              'print(m.predict_proba(sys.argv[2]).shape)')
    environment = dict(os.environ, PYTHONPATH=str(SOURCE.parents[2]))
    completed = subprocess.run([sys.executable, '-c', script, str(root / 'run'), str(data)],
                               cwd=root, env=environment, text=True, capture_output=True,
                               timeout=60)
    assert completed.returncode == 0, completed.stdout + completed.stderr
    assert completed.stdout.strip() == '(96, 2)'
    old_metrics = (root / 'run' / 'metrics.json').read_bytes()
    with unittest.TestCase().assertRaises(FileExistsError):
      model.fit((train_x, train_y), (val_x, val_y), root / 'run')
    assert (root / 'run' / 'metrics.json').read_bytes() == old_metrics
    np.testing.assert_allclose(model.predict_raw(val_x), raw, rtol=0, atol=0)


def test_python_api_projection():
  python_api_training('projection')


def test_python_api_single():
  python_api_training('single')


def test_python_api_vanga():
  python_api_training('vanga')


def test_module_cli_help():
  command = [sys.executable, '-S', '-m', 'vanga.utils.NodeTrainer.train', '--help']
  completed = subprocess.run(command, cwd=SOURCE.parents[2], text=True, capture_output=True,
                             timeout=30)
  assert completed.returncode == 0, completed.stdout + completed.stderr
  assert '--features-per-node' in completed.stdout


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--binary', type=Path, required=True)
  parser.add_argument('--device', default='cpu')
  arguments = parser.parse_args()
  BINARY = arguments.binary.resolve()
  DEVICE = trainer.choose_device(arguments.device)
  tests = [test_labels_and_cli, test_sparse_dense_gradients, test_raw_border_rounding,
           test_multibranch_additive_semantics, test_multibranch_native_export,
           test_projection_training_export, test_single_feature_training_export,
           test_multibranch_training_export, test_python_api_validation,
           test_python_api_projection, test_python_api_single, test_python_api_vanga,
           test_module_cli_help]
  suite = unittest.TestSuite(unittest.FunctionTestCase(test) for test in tests)
  outcome = unittest.TextTestRunner(verbosity=2).run(suite)
  raise SystemExit(0 if outcome.wasSuccessful() else 1)

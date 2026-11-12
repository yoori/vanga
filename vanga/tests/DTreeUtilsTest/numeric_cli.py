"""Exercise float training and resume through the same binary used for prediction."""

import math
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
  binary = Path(sys.argv[1]).resolve()

  def run(*args, success=True):
    result = subprocess.run([str(binary)] + [str(arg) for arg in args], text=True,
                            capture_output=True, timeout=30)
    assert (result.returncode == 0) == success, result.stdout + result.stderr
    return result

  def raw(model, data):
    return [float(value) for value in run('predict-raw', model, data).stdout.split()]

  def logloss(values, labels):
    return sum(max(x, 0) - y * x + math.log1p(math.exp(-abs(x)))
               for x, y in zip(values, labels)) / len(labels)

  with tempfile.TemporaryDirectory(prefix='vanga-numeric-cli-') as directory:
    root = Path(directory)
    data, model, best = root / 'train.libsvm', root / 'model', root / 'best'
    data.write_text(('0 7:0.1\n1 7:0.9\n') * 20)
    options = ['--steps=10', '--alpha=0.5', '--train-bags=0',
               '--metric-sqd=0', '--metric-logloss=1']
    model.write_text('obsolete')
    trained = run('train', model, data, data, '--save-best-model=' + str(best),
                  '--step-model-out=' + str(root / 'step-'), *options)
    assert 'numeric features' in trained.stdout and model.read_text().startswith('dtree-float\n')
    first = raw(model, data)
    assert first[0] < 0 < first[1]
    loss = logloss(first, [0, 1] * 20)
    assert loss < 0.3
    assert best.is_file() and list(root.glob('step-*.dtf'))
    printed = run('print', model, data)
    assert 'cover-logloss' in printed.stdout
    cover_loss = float(printed.stdout.split('cover-logloss = ')[1])
    assert abs(cover_loss - loss) < 1e-5
    assert logloss(raw(best, data), [0, 1] * 20) <= loss + 1e-12
    run('train-add', model, data, *options)
    assert logloss(raw(model, data), [0, 1] * 20) < loss
    ensemble = run('ensemble', model, model, data)
    values = dict(line.split(' = ') for line in ensemble.stdout.splitlines())
    assert abs(float(values['min_logloss']) - logloss(raw(model, data), [0, 1] * 20)) < 1e-6
    before_pruning = raw(model, data)
    run('filter', model, data, '--min-cover=0.4')
    assert raw(model, data) == before_pruning
    legacy = root / 'legacy'
    run('train', legacy, data, '--feature-type=binary', *options)
    binary_raw = raw(legacy, data)
    assert legacy.read_text().startswith('dtree\n') and len(set(binary_raw)) == 1
    zero_one = root / 'binary.libsvm'
    zero_one.write_text(('0\n1 7:1\n') * 20)
    annotated = root / 'annotated.libsvm'
    annotated.write_text(('0,-0.25\n1,0.25 7:1\n') * 20)
    annotated_model = root / 'annotated.model'
    run('train', annotated_model, annotated, *options)
    assert annotated_model.read_text().startswith('dtree\n')
    run('train-add', legacy, data, *options)
    assert legacy.read_text().startswith('dtree-float\n')
    forced = root / 'forced'
    run('train-trees', forced, zero_one, '--feature-type=float', *options)
    resumed = run('train-trees', forced, zero_one, *options)
    assert 'loaded model: numeric' in resumed.stdout
    assert forced.read_text().startswith('dtree-float\n')
    run('train-add', forced, zero_one, '--feature-type=binary', *options, success=False)
    nan_data, nan_model = root / 'nan.libsvm', root / 'nan.model'
    nan_data.write_text(('0\n1 4294967295:nan\n') * 20)
    run('train', nan_model, nan_data, '--threads=2', '--border-count=1', *options)
    assert raw(nan_model, nan_data)[0] < 0 < raw(nan_model, nan_data)[1]
    run('train-add', nan_model, nan_data, *options)
    assert raw(nan_model, nan_data)[0] < 0 < raw(nan_model, nan_data)[1]
    old = model.read_bytes()
    for row in ('0 7:inf\n', '2 7:0.5\n', '0 7:1 7:2\n', ''):
      bad = root / 'bad.libsvm'
      bad.write_text(row)
      run('train-add', model, bad, *options, success=False)
      assert model.read_bytes() == old
    for option in ('--border-count=0', '--numeric-l2=0', '--alpha=nan', '--depth=0',
                    '--feature-type=unknown', '--threads=0'):
      run('train-add', model, data, *options, option, success=False)
      assert model.read_bytes() == old
    assert not list(root.glob('*.partial'))


if __name__ == '__main__':
  main()

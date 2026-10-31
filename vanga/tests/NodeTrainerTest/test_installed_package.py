"""Run outside the repository with PYTHONPATH pointing to an installed wheel's target directory."""

import argparse
import importlib.metadata
from pathlib import Path
import tempfile

import numpy as np

import vanga
from vanga import Dataset, LibSvmFileDataset, VangaTrainer


def query_get_rows(self, count):
  position = getattr(self, 'position', 0)
  if position == 37:
    self.position = 0
    return None
  count = min(count, 37 - position, 5)
  rows = np.arange(position, position + count)
  values = np.column_stack((rows % 2, rows % 3, rows % 7, rows * 0)).astype(np.float32)
  self.position = position + count
  return values, (rows % 2).astype(np.int64)


class QueryDataset(Dataset):
  get_rows = query_get_rows


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--package-root', type=Path, required=True)
  args = parser.parse_args()
  assert Path(vanga.__file__).resolve().is_relative_to(args.package_root.resolve())
  assert importlib.metadata.version('vanga') == vanga.__version__
  with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    train, validation = QueryDataset(), QueryDataset()
    model = VangaTrainer(n_features=4, num_trees=2, max_depth=2, init_rows=7,
                         epochs=2, patience=0, batch_size=11, device='cpu', cpu_threads=1)
    model.fit(train, validation, root / 'run')
    assert model.metrics_['rows']['train'] == model.metrics_['rows']['validation'] == 37
    assert model.predict_proba(validation).shape == (37, 2)
    restored = VangaTrainer.load(root / 'run')
    np.testing.assert_array_equal(model.predict_raw(validation), restored.predict_raw(validation))
    data = root / 'test.libsvm'
    data.write_text('0 1:0\n1 1:1\n')
    with LibSvmFileDataset(data, n_features=4) as source:
      assert source.get_rows(9)[0].shape == (2, 4)
      assert source.get_rows(9) is None
      assert source.get_rows(9)[0].shape == (2, 4)
  print('Installed vanga: imports, custom Dataset, training, reload and LibSVM restart passed')

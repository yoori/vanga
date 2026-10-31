#!/usr/bin/env python3
r"""Train sparse-input trees on GPU and export hardened models for Vanga::DTree.

Example:
  python train.py --train train.libsvm.gz --validation validation.libsvm.gz \
    --output-dir run --num-trees 100 --max-depth 6 --device cuda --feature-mode single

With --feature-mode single, splits use original LibSVM feature IDs and exported
JSON requires no preprocessing. Projection mode retains the original NODE
architecture and saves node_projection_transform.npz for prediction.

--feature-mode vanga --features-per-node 2 trains additive multi-branch nodes:
  node(x) = sum_k (yes_child_k(x) if x[feature_k] > border_k else no_child_k(x)).
Each tree shares its K distinct feature checks across nodes of the same level.
Features, thresholds and leaf values are learned; no linear projection is used.
This mode exports node_hardened.vanga, ready for DTreeTrainer predict[-raw].
The full topology grows as (2*K)**depth; --max-model-nodes limits its size.
"""

import argparse
from pathlib import Path

if __package__:
  from .config import validate_settings
else:
  from config import validate_settings


def positive_int(value):
  number = int(value)
  if number <= 0:
    raise argparse.ArgumentTypeError('must be positive')
  return number


def parse_args(argv=None):
  parser = argparse.ArgumentParser(description=__doc__,
                                   formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument('--train', type=Path, required=True, help='one-based LibSVM or .gz')
  parser.add_argument('--validation', type=Path, required=True, help='one-based LibSVM or .gz')
  parser.add_argument('--output-dir', type=Path, required=True, help='new output directory')
  parser.add_argument('--num-trees', type=positive_int, default=100)
  parser.add_argument('--max-depth', '--depth', dest='max_depth', type=positive_int, default=6,
                      help='tree depth, 1..16; every symmetric tree uses all levels')
  parser.add_argument('--n-features', type=positive_int, default=16384)
  parser.add_argument('--epochs', type=positive_int, default=10)
  parser.add_argument('--patience', type=int, default=3,
                      help='epochs without improvement; 0 disables')
  parser.add_argument('--batch-size', type=positive_int, default=1024)
  parser.add_argument('--eval-batch-size', type=positive_int, help='defaults to --batch-size')
  parser.add_argument('--init-rows', type=positive_int, default=1000,
                      help='maximum dense training rows used for data-aware initialization')
  parser.add_argument('--learning-rate', type=float, default=1e-3)
  parser.add_argument('--seed', type=int, default=0)
  parser.add_argument('--cpu-threads', type=positive_int, default=4)
  parser.add_argument('--device', default='cuda',
                      help='cuda, cuda:INDEX, or cpu; no implicit fallback')
  parser.add_argument('--input-mode', choices=('sparse', 'dense'), default='sparse')
  parser.add_argument('--feature-mode', choices=('projection', 'single', 'vanga'),
                      default='projection', help='projection, single or additive Vanga nodes')
  parser.add_argument('--features-per-node', type=positive_int,
                      help='distinct feature branches per node in vanga mode; default 2')
  parser.add_argument('--max-model-nodes', type=positive_int, default=1000000,
                      help='maximum total topology nodes in vanga mode; default 1000000')
  parser.add_argument('--log-every-batches', type=int, default=1000, help='0 logs only epochs')
  args = parser.parse_args(argv)
  if args.features_per_node is not None and args.feature_mode != 'vanga':
    parser.error('--features-per-node requires --feature-mode vanga')
  try:
    args = argparse.Namespace(**validate_settings(vars(args)))
  except ValueError as error:
    parser.error(str(error))
  for path in (args.train, args.validation):
    if not path.is_file():
      parser.error('input file does not exist: ' + str(path))
  if args.output_dir.exists():
    parser.error('--output-dir already exists: ' + str(args.output_dir))
  args.eval_batch_size = args.eval_batch_size or args.batch_size
  args.train = args.train.resolve()
  args.validation = args.validation.resolve()
  args.output_dir = args.output_dir.resolve()
  return args


def main(argv=None):
  args = parse_args(argv)
  try:
    if __package__:
      from .trainer import train
    else:
      import sys
      sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
      from vanga.utils.NodeTrainer.trainer import train
  except ImportError as error:
    raise SystemExit('Install NodeTrainer/requirements.txt and a compatible PyTorch: ' + str(error))
  try:
    train(args)
  except Exception as error:
    raise SystemExit(type(error).__name__ + ': ' + str(error))


if __name__ == '__main__':
  main()

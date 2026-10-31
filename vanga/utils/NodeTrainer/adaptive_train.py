#!/usr/bin/env python3
"""Adaptive Vanga training: NODE growth, CPU pruning and validation selection."""

import argparse
import math
from pathlib import Path


def positive_int(value):
  number = int(value)
  if number <= 0:
    raise argparse.ArgumentTypeError('must be positive')
  return number


def parse_args(argv=None):
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--primary-train', type=Path, required=True,
                      help='LibSVM input for gradient training')
  parser.add_argument('--secondary-train', type=Path, required=True,
                      help='independent LibSVM input for NODE early stopping and pruning')
  parser.add_argument('--validation', type=Path, required=True,
                      help='LibSVM input for step selection and outer stopping')
  parser.add_argument('--output-dir', type=Path, required=True)
  parser.add_argument('--n-features', type=positive_int, default=16384)
  parser.add_argument('--mutation-nodes', type=positive_int, default=15,
                      help='X: nodes added by inplace; extend adds X + Y')
  parser.add_argument('--extend-extra-nodes', type=positive_int, default=5,
                      help='Y: extra nodes added by extend')
  parser.add_argument('--epochs', type=positive_int, default=10)
  parser.add_argument('--node-patience', type=positive_int, default=3)
  parser.add_argument('--topology-patience', type=positive_int, default=5)
  parser.add_argument('--min-improvement', type=float, default=0.05,
                      help='relative loss improvement required to reset patience')
  parser.add_argument('--batch-size', type=positive_int, default=1024)
  parser.add_argument('--eval-batch-size', type=positive_int)
  parser.add_argument('--init-rows', type=positive_int, default=1000)
  parser.add_argument('--learning-rate', type=float, default=1e-3)
  parser.add_argument('--seed', type=int, default=0)
  parser.add_argument('--cpu-threads', type=positive_int, default=4)
  parser.add_argument('--device', default='cuda', help='cuda, cuda:INDEX or cpu')
  parser.add_argument('--input-mode', choices=('sparse', 'dense'), default='sparse')
  parser.add_argument('--log-every-batches', type=int, default=1000)
  parser.add_argument('--quiet', action='store_true')
  args = parser.parse_args(argv)
  if not math.isfinite(args.min_improvement) or not 0 <= args.min_improvement < 1:
    parser.error('--min-improvement must be in [0, 1)')
  if args.seed < 0 or args.log_every_batches < 0:
    parser.error('--seed and --log-every-batches must be nonnegative')
  for path in (args.primary_train, args.secondary_train, args.validation):
    if not path.is_file():
      parser.error('input file does not exist: ' + str(path))
  if args.output_dir.exists():
    parser.error('--output-dir already exists: ' + str(args.output_dir))
  return args


def main(argv=None):
  args = parse_args(argv)
  from .adaptive_trainer import VangaTrainer
  trainer = VangaTrainer(
    n_features=args.n_features,
    mutation_nodes=args.mutation_nodes,
    extend_extra_nodes=args.extend_extra_nodes,
    epochs=args.epochs,
    node_patience=args.node_patience,
    topology_patience=args.topology_patience,
    min_improvement=args.min_improvement,
    batch_size=args.batch_size,
    eval_batch_size=args.eval_batch_size,
    init_rows=args.init_rows,
    learning_rate=args.learning_rate,
    seed=args.seed,
    cpu_threads=args.cpu_threads,
    device=args.device,
    input_mode=args.input_mode,
    log_every_batches=args.log_every_batches,
    verbose=not args.quiet)
  try:
    trainer.fit(args.primary_train, args.secondary_train, args.validation, args.output_dir)
  except Exception as error:
    raise SystemExit(type(error).__name__ + ': ' + str(error))
  return 0


if __name__ == '__main__':
  raise SystemExit(main())

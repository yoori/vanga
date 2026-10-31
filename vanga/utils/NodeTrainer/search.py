#!/usr/bin/env python3
"""Search nearby Vanga tree sizes by validation logloss and exported node count.

Example (run on nvidia00; GPU 0 is intentionally never used):
  python -m vanga.utils.NodeTrainer.search \
    --train train.libsvm.gz --validation validation.libsvm.gz \
    --output-dir node-search --n-features 16384 --device-gpus 1,2,3

The search starts at 64 trees, depth 4, and 2 features per node. It evaluates
the center and up to eight neighboring grid points, combining one step up or
down on each of the three axes. The best configuration under
logloss + penalty * log2(nodes / center_nodes) becomes the next center.
Each candidate is a fresh invocation of the existing NodeTrainer CLI.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from itertools import product
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import time


def positive_int(value):
  number = int(value)
  if number <= 0:
    raise argparse.ArgumentTypeError('must be positive')
  return number


def parse_gpus(value):
  try:
    result = tuple(dict.fromkeys(int(part) for part in value.split(',')))
  except ValueError as error:
    raise argparse.ArgumentTypeError('expected comma-separated GPU indexes') from error
  if not result or any(index < 0 for index in result):
    raise argparse.ArgumentTypeError('GPU indexes must be nonnegative')
  if 0 in result:
    raise argparse.ArgumentTypeError('GPU 0 is reserved and will not be used')
  return result


def parse_args(argv=None):
  parser = argparse.ArgumentParser(description=__doc__,
                                   formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument('--train', type=Path, required=True)
  parser.add_argument('--validation', type=Path, required=True,
                      help='primary validation used during search; keep blind validation separate')
  parser.add_argument('--output-dir', type=Path, required=True,
                      help='new directory for candidate runs and search.json')
  parser.add_argument('--n-features', type=positive_int, required=True)
  parser.add_argument('--base-num-trees', type=positive_int, default=64)
  parser.add_argument('--base-depth', type=positive_int, default=4)
  parser.add_argument('--base-features-per-node', type=positive_int, default=2)
  parser.add_argument('--device-gpus', type=parse_gpus, default=(1, 2, 3),
                      help='physical GPU indexes; GPU 0 is rejected (default: 1,2,3)')
  parser.add_argument('--rounds', type=positive_int, default=3,
                      help='maximum number of center-and-neighbor search rounds')
  parser.add_argument('--complexity-penalty', type=float, default=0.001,
                      help='logloss cost accepted per doubling of exported node count')
  parser.add_argument('--max-num-trees', type=positive_int, default=4096)
  parser.add_argument('--max-depth', type=positive_int, default=16)
  parser.add_argument('--max-model-nodes', type=positive_int, default=1000000)
  parser.add_argument('--epochs', type=positive_int, default=10)
  parser.add_argument('--patience', type=int, default=3)
  parser.add_argument('--batch-size', type=positive_int, default=1024)
  parser.add_argument('--eval-batch-size', type=positive_int)
  parser.add_argument('--init-rows', type=positive_int, default=1000)
  parser.add_argument('--learning-rate', type=float, default=1e-3)
  parser.add_argument('--seed', type=int, default=0)
  parser.add_argument('--cpu-threads', type=positive_int, default=4)
  parser.add_argument('--input-mode', choices=('sparse', 'dense'), default='sparse')
  parser.add_argument('--log-every-batches', type=int, default=1000)
  args = parser.parse_args(argv)
  if not math.isfinite(args.complexity_penalty) or args.complexity_penalty < 0:
    parser.error('--complexity-penalty must be finite and nonnegative')
  if args.patience < 0 or args.log_every_batches < 0:
    parser.error('--patience and --log-every-batches must be nonnegative')
  if args.base_depth > args.max_depth:
    parser.error('--base-depth cannot exceed --max-depth')
  if args.base_num_trees > args.max_num_trees:
    parser.error('--base-num-trees cannot exceed --max-num-trees')
  if args.base_features_per_node > args.n_features:
    parser.error('--base-features-per-node cannot exceed --n-features')
  if args.max_depth > 16:
    parser.error('--max-depth must be at most 16')
  if args.base_num_trees * nodes_per_tree(args.base_depth, args.base_features_per_node) > \
      args.max_model_nodes:
    parser.error('base model exceeds --max-model-nodes')
  for path in (args.train, args.validation):
    if not path.is_file():
      parser.error('input file does not exist: ' + str(path))
  if args.output_dir.exists():
    parser.error('--output-dir already exists: ' + str(args.output_dir))
  args.train = args.train.resolve()
  args.validation = args.validation.resolve()
  args.output_dir = args.output_dir.resolve()
  return args


def nodes_per_tree(depth, features_per_node):
  width = 2 * features_per_node
  return (width ** (depth + 1) - 1) // (width - 1)


def model_nodes(num_trees, depth, features_per_node):
  # Trees share one root in the exported native Vanga model.
  return 1 + num_trees * (nodes_per_tree(depth, features_per_node) - 1)


def topology_nodes(num_trees, depth, features_per_node):
  # Match NodeTrainer's pre-export topology limit (it counts each tree root).
  return num_trees * nodes_per_tree(depth, features_per_node)


def config_key(config):
  return (config['num_trees'], config['depth'], config['features_per_node'])


def neighbors(center, args):
  trees = center['num_trees']
  depth = center['depth']
  branches = center['features_per_node']
  unique = {}
  for count, candidate_depth, candidate_branches in product(
      (max(1, trees // 2), min(args.max_num_trees, trees * 2)),
      (max(1, depth - 1), min(args.max_depth, depth + 1)),
      (max(1, branches - 1), min(args.n_features, branches + 1))):
    candidate = {'num_trees': count, 'depth': candidate_depth,
                 'features_per_node': candidate_branches}
    key = config_key(candidate)
    if key != config_key(center):
      unique[key] = candidate
  return list(unique.values())


def candidate_score(metrics, penalty, center_nodes):
  loss = metrics['best_validation_logloss']
  nodes = metrics['export']['nodes']
  return loss + penalty * math.log2(nodes / center_nodes)


def candidate_name(config):
  return 'trees-{}__depth-{}__features-{}'.format(
    config['num_trees'], config['depth'], config['features_per_node'])


def write_json(path, value):
  partial = path.with_name(path.name + '.partial')
  partial.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')
  partial.replace(path)


def train_candidate(config, gpu, args):
  name = candidate_name(config)
  output = args.output_dir / 'candidates' / name
  output.mkdir(parents=True, exist_ok=False)
  log_path = output / 'train.log'
  command = [sys.executable, str(Path(__file__).with_name('train.py')),
             '--train', str(args.train), '--validation', str(args.validation),
             '--output-dir', str(output / 'run'), '--feature-mode', 'vanga',
             '--n-features', str(args.n_features), '--num-trees', str(config['num_trees']),
             '--max-depth', str(config['depth']), '--features-per-node',
             str(config['features_per_node']), '--max-model-nodes', str(args.max_model_nodes),
             '--epochs', str(args.epochs), '--patience', str(args.patience),
             '--batch-size', str(args.batch_size), '--init-rows', str(args.init_rows),
             '--learning-rate', str(args.learning_rate), '--seed', str(args.seed),
             '--cpu-threads', str(args.cpu_threads), '--device', 'cuda',
             '--input-mode', args.input_mode,
             '--log-every-batches', str(args.log_every_batches)]
  if args.eval_batch_size is not None:
    command.extend(('--eval-batch-size', str(args.eval_batch_size)))

  started = time.monotonic()
  environment = os.environ.copy()
  environment['CUDA_VISIBLE_DEVICES'] = str(gpu)
  environment['CUDA_DEVICE_ORDER'] = 'PCI_BUS_ID'
  with log_path.open('w') as log:
    process = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                             env=environment, check=False)
  result = {'config': config, 'gpu': gpu, 'return_code': process.returncode,
            'elapsed_seconds': time.monotonic() - started,
            'output_dir': str(output / 'run'), 'log': str(log_path)}
  metrics_path = output / 'run' / 'metrics.json'
  if process.returncode == 0 and metrics_path.is_file():
    metrics = json.loads(metrics_path.read_text())
    if metrics.get('status') == 'complete':
      result.update(status='complete',
                    validation_logloss=metrics['best_validation_logloss'],
                    nodes=metrics['export']['nodes'],
                    model_file_bytes=(output / 'run' / 'node_hardened.vanga').stat().st_size,
                    best_epoch=metrics['best_epoch'])
      return result
    result.update(status='failed', error='training did not complete')
  else:
    result.update(status='failed', error='NodeTrainer exited with code {}'.format(
      process.returncode))
  return result


def main(argv=None):
  args = parse_args(argv)
  args.output_dir.mkdir(parents=True)
  (args.output_dir / 'candidates').mkdir()
  base = {'num_trees': args.base_num_trees, 'depth': args.base_depth,
          'features_per_node': args.base_features_per_node}
  center_nodes = model_nodes(**{
    'num_trees': base['num_trees'], 'depth': base['depth'],
    'features_per_node': base['features_per_node']})
  search = {'status': 'running', 'host': os.uname().nodename, 'gpus': list(args.device_gpus),
            'center': base, 'center_nodes': center_nodes,
            'complexity_penalty': args.complexity_penalty,
            'score': 'validation_logloss + penalty * log2(nodes / center_nodes)',
            'settings': {'train': str(args.train), 'validation': str(args.validation),
                         'n_features': args.n_features, 'seed': args.seed,
                         'epochs': args.epochs, 'patience': args.patience},
            'rounds': [], 'candidates': {}}
  write_json(args.output_dir / 'search.json', search)
  evaluated = {}
  center = base

  for round_number in range(1, args.rounds + 1):
    round_configs = [center] + neighbors(center, args)
    pending = [config for config in round_configs if config_key(config) not in evaluated]
    pending = [config for config in pending
               if topology_nodes(config['num_trees'], config['depth'],
                                 config['features_per_node']) <= args.max_model_nodes]
    if not pending:
      break
    round_record = {'round': round_number, 'center': center, 'candidates': []}
    with ThreadPoolExecutor(max_workers=len(args.device_gpus)) as pool:
      futures = {}
      for index, config in enumerate(pending):
        gpu = args.device_gpus[index % len(args.device_gpus)]
        future = pool.submit(train_candidate, config, gpu, args)
        futures[future] = config
      for future in as_completed(futures):
        config = futures[future]
        try:
          result = future.result()
        except Exception as error:
          result = {'config': config, 'status': 'failed', 'error': str(error)}
        evaluated[config_key(config)] = result
        if result.get('status') == 'complete':
          result['score'] = candidate_score(
            {'best_validation_logloss': result['validation_logloss'],
             'export': {'nodes': result['nodes']}},
            args.complexity_penalty, center_nodes)
        search['candidates'][candidate_name(config)] = result
        round_record['candidates'].append(candidate_name(config))
        write_json(args.output_dir / 'search.json', search)
        print(json.dumps({'event': 'candidate_finished', **result}), flush=True)
    successful = [value for value in evaluated.values() if value.get('status') == 'complete']
    if not successful:
      search.update(status='failed', error='all candidate trainings failed')
      write_json(args.output_dir / 'search.json', search)
      return 1
    winner = min(successful, key=lambda value: value['score'])
    winner_config = winner['config']
    round_record['winner'] = winner_config
    round_record['winner_score'] = winner['score']
    search['rounds'].append(round_record)
    center = winner_config
    search['best'] = winner
    write_json(args.output_dir / 'search.json', search)

  search.update(status='complete', best=min(
    (value for value in evaluated.values() if value.get('status') == 'complete'),
    key=lambda value: value['score']))
  write_json(args.output_dir / 'search.json', search)
  print(json.dumps({'event': 'search_finished', 'best': search['best'],
                    'search_json': str(args.output_dir / 'search.json')}, indent=2), flush=True)
  return 0


if __name__ == '__main__':
  raise SystemExit(main())

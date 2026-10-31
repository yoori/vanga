"""Vanga tree training with incremental, user-defined data sources."""

__version__ = '0.1.0'
__all__ = ['Dataset', 'LibSvmFileDataset', 'ArrayDataset', 'VangaTrainer', 'NodeTrainer',
           'AdaptiveVangaTrainer']


def __getattr__(name):
  if name in ('Dataset', 'LibSvmFileDataset', 'ArrayDataset'):
    from . import dataset
    return getattr(dataset, name)
  if name == 'VangaTrainer':
    from .trainer import VangaTrainer
    return VangaTrainer
  if name in ('NodeTrainer', 'AdaptiveVangaTrainer'):
    from .trainer import NodeTrainer, AdaptiveVangaTrainer
    return NodeTrainer if name == 'NodeTrainer' else AdaptiveVangaTrainer
  raise AttributeError('module {} has no attribute {}'.format(__name__, name))

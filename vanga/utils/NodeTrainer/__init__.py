"""NODE training APIs: fixed-topology and adaptive Vanga training."""

__all__ = ['VangaTrainer', 'NodeTrainer', 'AdaptiveVangaTrainer']


def __getattr__(name):
  if name == 'VangaTrainer':
    from .trainer import VangaTrainer
    return VangaTrainer
  if name in ('NodeTrainer', 'AdaptiveVangaTrainer'):
    from .adaptive_trainer import NodeTrainer, VangaTrainer
    return NodeTrainer if name == 'NodeTrainer' else VangaTrainer
  raise AttributeError('module {} has no attribute {}'.format(__name__, name))

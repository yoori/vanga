"""NODE training APIs: fixed-topology and adaptive Vanga training."""

__all__ = ['VangaTrainer', 'NodeTrainer', 'AdaptiveVangaTrainer', 'IndependentVangaTrainer']


def __getattr__(name):
  if name == 'VangaTrainer':
    from .trainer import VangaTrainer
    return VangaTrainer
  if name in ('NodeTrainer', 'AdaptiveVangaTrainer'):
    from .adaptive_trainer import NodeTrainer, VangaTrainer
    return NodeTrainer if name == 'NodeTrainer' else VangaTrainer
  if name == 'IndependentVangaTrainer':
    from .independent_trainer import IndependentVangaTrainer
    return IndependentVangaTrainer
  raise AttributeError('module {} has no attribute {}'.format(__name__, name))

"""Install, relocate and consume Vanga using only its exported CMake target."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def run(command, environment):
  subprocess.run([str(value) for value in command], env=environment, check=True)


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  for name in ('cmake', 'ctest', 'build-dir', 'config', 'generator', 'compiler', 'libdir'):
    parser.add_argument('--' + name, required=True)
  args = parser.parse_args()
  build = Path(args.build_dir).resolve()
  environment = os.environ.copy()
  for name in ('DESTDIR', 'LD_LIBRARY_PATH', 'LIBRARY_PATH', 'CPATH', 'CPLUS_INCLUDE_PATH'):
    environment.pop(name, None)
  with tempfile.TemporaryDirectory(prefix='vanga-installed-', dir=build) as directory:
    root = Path(directory)
    original, installed = root / 'original', root / 'relocated'
    run([args.cmake, '--install', build, '--config', args.config,
         '--prefix', original], environment)
    original.rename(installed)
    config = installed / args.libdir / 'cmake' / 'Vanga'
    assert (config / 'VangaConfig.cmake').is_file()
    assert (config / 'VangaConfigVersion.cmake').is_file()
    for path in config.glob('*.cmake'):
      contents = path.read_text()
      assert str(build) not in contents, path
      assert str(original) not in contents, path
    source = root / 'consumer'
    shutil.copytree(Path(__file__).parent / 'consumer', source)
    consumer_build = root / 'consumer-build'
    run([args.cmake, '-S', source, '-B', consumer_build, '-G', args.generator,
         '-DCMAKE_CXX_COMPILER=' + args.compiler, '-DCMAKE_BUILD_TYPE=' + args.config,
         '-DCMAKE_PREFIX_PATH=' + str(installed)], environment)
    run([args.cmake, '--build', consumer_build, '--config', args.config], environment)
    run([args.ctest, '--test-dir', consumer_build, '-C', args.config,
         '--output-on-failure'], environment)
    model, data = root / 'model.vanga', root / 'input.libsvm'
    model.write_text('dtree-float\n1\t0.25\t7:2:3:1.5:0\n2\t0.5\t\n3\t-0.5\t\n')
    data.write_text('0\n1 7:2\n')
    prediction = subprocess.run(
      [str(installed / 'bin' / 'DTreeTrainer'), 'predict-raw', str(model), str(data)],
      env=environment, check=True, text=True, capture_output=True)
    assert [float(value) for value in prediction.stdout.split()] == [-0.25, 0.75]


if __name__ == '__main__':
  main()

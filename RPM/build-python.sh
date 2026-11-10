#!/bin/bash
set -euo pipefail

if [ "$#" -ne 0 ]; then
  echo "Usage: PYTHON=python3.12 OUTPUT_DIR=build/RPMS $0" >&2
  exit 1
fi
script_dir=$(cd "$(dirname "$0")" && pwd)
source_root=$(cd "$script_dir/.." && pwd)
python_root="$source_root/python"
build_python=${PYTHON:-python3.12}
"$build_python" -c 'import sys; assert sys.version_info[:2] == (3, 12), "Python 3.12 required"'
version=$("$build_python" -c \
  'import sys,tomllib; print(tomllib.load(open(sys.argv[1], "rb"))["project"]["version"])' \
  "$python_root/pyproject.toml")
top=$(mktemp -d "${TMPDIR:-/tmp}/python3-vanga-rpmbuild.XXXXXXXX")
trap 'rm -rf "$top"' EXIT
mkdir -p "$top"/{BUILD,BUILDROOT,RPMS,SOURCES,SRPMS}
tar -C "$source_root" -czf "$top/SOURCES/vanga-$version.tar.gz" \
  --exclude='python/tests' --exclude='__pycache__' --exclude='*.pyc' \
  --transform="s,^python,vanga-$version," python
rpmbuild --define "_topdir $top" --define "_tmppath ${TMPDIR:-/tmp}" \
  --define "vanga_version $version" -ba "$script_dir/SPECS/python3-vanga.spec"
output=${OUTPUT_DIR:-$source_root/build/RPMS}
mkdir -p "$output/SRPMS"
cp "$top"/RPMS/noarch/python3-vanga-*.rpm "$output/"
cp "$top"/SRPMS/python3-vanga-*.rpm "$output/SRPMS/"
echo "RPM output: $output"

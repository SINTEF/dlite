#!/bin/bash
# Help script for test_docker.sh

# Exit on errors and treat unbound variables as errors
set -eu

#PY_MINORS="10 11 12 13 14"
PY_MINORS="10 14"


for minor in $PY_MINORS; do

    python=python3.$minor
    tmpdir=/tmp/cp3${minor}
    venvdir=$tmpdir/venv
    builddir=$tmpdir/build

    # Activate virtual environment
    source $venvdir/bin/activate

    # Build, install and test DLite
    mkdir -p $builddir
    cd $builddir
    cmake -DCMAKE_INSTALL_PREFIX=$tmpdir /io
    #cmake -DPYTHON_VERSION=3.$minor /io
    cmake --build . -j4
    cmake --install .
    ctest -j4
done

#!/bin/bash
# Help script for test_docker.sh

# Exit on errors and treat unbound variables as errors
set -eu

echo "SYSTEM=$SYSTEM"
echo "TYPE=$TYPE"
echo "ARCH=$ARCH"
echo "PY_MINORS=$PY_MINORS"

for minor in $PY_MINORS; do

    python=python3.$minor
    tmpdir=/tmp/dlite/cp3${minor}-${SYSTEM}${TYPE}_${ARCH}
    builddir=$tmpdir/build

    mkdir -p $builddir

    # Create virtual environment
    #venvdir=$tmpdir/venv
    #$python -m venv $venvdir
    #source $venvdir/bin/activate

    # Build, install and test DLite
    cd $builddir
    #cmake -DCMAKE_INSTALL_PREFIX=$tmpdir /io
    cmake -DPYTHON_VERSION=3.$minor /io
    cmake --build . -j4
    cmake --install .
    ctest -j4
done

#!/bin/bash
# Help script for test_docker.sh

# Exit on errors and treat unbound variables as errors
set -eu

#PY_MINORS="${PY_MINORS:-10 14}"
PY_MINORS="${PY_MINORS:-14}"


for minor in $PY_MINORS; do
    echo "======================================================="
    echo "===  Test for Python 3.$minor"
    echo "======================================================="

    python=python3.$minor
    tmpdir=/tmp/cp3${minor}
    venvdir=$tmpdir-venv
    builddir=$tmpdir-build

    # Activate virtual environment
    source $venvdir/bin/activate

    # Build, install and test DLite
    mkdir -p $builddir
    cd $builddir

    echo
    echo "=== Python 3.$minor: configure ==="
    cmake -DCMAKE_INSTALL_PREFIX=$tmpdir /io

    echo
    echo "=== Python 3.$minor: build ==="
    cmake --build . -j
    cmake --install .

    echo
    echo "=== Python 3.$minor: Run all tests ==="
    ctest -j || ctest --rerun-failed --output-on-failure -V

    exit  # XXX

    echo
    echo "=== Python 3.$minor: Test with all behavior changes disabled ==="
    export DLITE_BEHAVIOR=OFF
    ctest -j4 -E static-code-analysis -E test_python_bindings-py || ctest --rerun-failed --output-on-failure -V

    echo
    echo "=== Python 3.$minor: Test with all behavior changes enabled ==="
    export DLITE_BEHAVIOR=ON
    ctest -j4 -E static-code-analysis -E test_python_bindings-py || ctest --rerun-failed --output-on-failure -V

done

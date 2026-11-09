#!/bin/bash
# Help script for test_docker.sh

# Exit on errors and treat unbound variables as errors
set -eu

# Python minor versions to test.  Defaults to the newest Python for which
# the docker image has a virtual environment (/tmp/cp3<minor>-venv), so
# that this script doesn't need updating when the image's Python versions
# change.  Set PY_MINORS (e.g. "9 15") to test other or several versions.
if [ -z "${PY_MINORS:-}" ]; then
    PY_MINORS=$(ls -d /tmp/cp3*-venv 2>/dev/null \
                    | sed -E 's|^/tmp/cp3([0-9]+)-venv$|\1|' | sort -n | tail -1)
    if [ -z "$PY_MINORS" ]; then
        echo "No /tmp/cp3*-venv virtual environment found" >&2
        exit 1
    fi
fi


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

    echo
    echo "=== Python 3.$minor: Test with all behavior changes disabled ==="
    export DLITE_BEHAVIOR=OFF
    ctest -j -E static-code-analysis -E test_python_bindings-py || ctest --rerun-failed --output-on-failure -V

    echo
    echo "=== Python 3.$minor: Test with all behavior changes enabled ==="
    export DLITE_BEHAVIOR=ON
    ctest -j -E static-code-analysis -E test_python_bindings-py -E test_dataset2_load-py || ctest --rerun-failed --output-on-failure -V

done

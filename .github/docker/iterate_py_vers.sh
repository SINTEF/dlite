#!/bin/bash
## Install requirements for different Python 3 minor versions.
# Usage:
#   iterate_py_vers MINOR_PY3_VER ...
set -eux

ARCH=x86_64
PLAT=manylinux2014_${ARCH}

for d in /opt/python/cp*; do
    set +x
    TAG=${d##*/}
    x=${TAG%-*}
    MINOR=${x#cp3}
    PYVER=3.$MINOR
    ABI=${TAG#*-}
    ABIFLAGS=$(echo $ABI | sed 's/cp[0-9]*//')
    echo "TAG=$TAG"
    echo "PYVER=$PYVER"
    echo "ABI=$ABI"
    echo "ABIFLAGS=$ABIFLAGS"
    echo "MINOR=$MINOR"

    # Exit if minor version is not in argument list
    ok=false
    for minor in "$@"; do
        [ $minor = $MINOR ] && ok=true
    done
    $ok || exit
    set -x

    BUILDDIR=/io/python/build/temp.${PLAT}-${PYVER}
    INSTALLDIR=/io/python/build/lib.${PLAT}-${PYVER}

    CMAKE=/opt/python/${TAG}/bin/cmake
    Python3_EXECUTABLE=/opt/python/${TAG}/bin/python${PYVER}
    Python3_LIBRARY=/opt/python/${TAG}/lib/libpython${PYVER}.a
    Python3_INCLUDE_DIR=/opt/python/${TAG}/include/python${PYVER}

    # Install requirements
    ${Python3_EXECUTABLE} -m pip install -U pip
    ${Python3_EXECUTABLE} -m pip install -U setuptools wheel build CMake
    ${Python3_EXECUTABLE} -m pip install -r /tmp/requirements.txt
done

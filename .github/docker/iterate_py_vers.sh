#!/bin/bash
## Install requirements for different Python 3 minor versions.
# Usage:
#   iterate_py_vers MINOR_PY3_VER ...
set -eux

ARCH=x86_64
PLAT=manylinux2014_${ARCH}

for MINOR in "$@"; do
    # Python version and tag
    PYVER=3.${MINOR}
    PYTAG=cp3${MINOR}

    # Setup version-specific information
    PYABI=${PYTAG}
    ABITAG=${PYTAG}-${PYABI}

    BUILDDIR=/io/python/build/temp.${PLAT}-${PYVER}
    INSTALLDIR=/io/python/build/lib.${PLAT}-${PYVER}

    CMAKE=/opt/python/${ABITAG}/bin/cmake
    Python3_EXECUTABLE=/opt/python/${ABITAG}/bin/python${PYVER}
    Python3_LIBRARY=/opt/python/${ABITAG}/lib/libpython${PYVER}.a
    Python3_INCLUDE_DIR=/opt/python/${ABITAG}/include/python${PYVER}

    # Install requirements
    ${Python3_EXECUTABLE} -m pip install -U pip
    ${Python3_EXECUTABLE} -m pip install -U setuptools wheel build CMake
    ${Python3_EXECUTABLE} -m pip install -r /tmp/requirements.txt
done

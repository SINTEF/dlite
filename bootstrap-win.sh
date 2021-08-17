#
# Check and modify the path of the variables below.
#
# To play nice with git, leave this file untouched and add your updated
# variables to bootstrap-win-user.sh
#
# Run this script with (tested with git bash)
#
#     sh bootstrap-win.sh
#
#
# Init environment variables
#
GENERATOR="Visual Studio 15 2017 Win64"
BUILD_DIR=build-VS
ROOT_PATH="$(realpath $(pwd)/..)"
PATH="$CMAKE_PATH:$SWIG_PATH:$MAKE_PROGRAM_PATH:$ROOT_PATH/local/bin:$PATH"
INSTALL_PREFIX="$ROOT_PATH/local"

CMAKE_PATH="/c/Program Files/CMake/bin/cmake"

WITH_HDF5=OFF
HDF5_VERSION="1.10.4"

WITH_PYTHON=ON
SWIG_DIR="$INSTALL_PREFIX/swigwin-4.0.2/Lib"
SWIG_EXECUTABLE="$INSTALL_PREFIX/swigwin-4.0.2/swig"

SWIG_PATH="$HOME/source/repos/local/swigwin-4.0.1"


#
# Setup the shell
#
set -e  # Stop on first error
set -x  # Print commands as they are executed


#
# You normally don't need to modify the lines below
# =================================================

#
# Source user defaults from bootstrap-win-user.sh
#
if [ -f bootstrap-win-user.sh ]; then
    source bootstrap-win-user.sh
fi
#
# Build and install hdf5
#
if [ ! "$WITH_HDF5" = OFF ] && [ ! -f $INSTALL_PREFIX/lib/libhdf5.lib ]; then
    cd $ROOT_PATH
    if [ ! -d hdf5-$HDF5_VERSION ]; then
        TARFILE=hdf5-$HDF5_VERSION.tar.gz
        if [ ! -f $TARFILE ]; then
            curl https://support.hdfgroup.org/ftp/HDF5/current/src/$TARFILE \
                 --output $TARFILE
        fi
        tar -zxvf $TARFILE
    fi
    cd hdf5-$HDF5_VERSION
    mkdir -p ${BUILD_DIR} && cd ${BUILD_DIR}
    cmake \
        -G "$GENERATOR" \
        -DCMAKE_INSTALL_PREFIX:PATH="$INSTALL_PREFIX" \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_SHARED_LIBS=OFF \
        ..
    cmake --build . --config Debug --target install
    cmake --build . --config Release --target install
fi


#
# Configure and generate dlite project
#
cd $ROOT_PATH/dlite
git submodule update --init
mkdir -p ${BUILD_DIR} && cd ${BUILD_DIR}
#
# Set up environment
config=Release
export PATH="src/$config:src/utils/$config:$PATH"
export LD_LIBRARY_PATH="src/$config:src/utils/$config:$LD_LIBRARY_PATH"
export PYTHONPATH="bindings/python/dlite"
export DLITE_STORAGE_PLUGIN_DIRS="storages/json/$config:storages/hdf5/$config:storages/python/$config"
export DLITE_MAPPING_PLUGIN_DIRS="src/pyembed"
export DLITE_PYTHON_STORAGE_PLUGIN_DIRS="storages/python/python-storage-plugins"
export DLITE_PYTHON_MAPPING_PLUGIN_DIRS="bindings/python/mapping-plugins"
export DLITE_TEMPLATE_DIRS="../tools/templates"
export DLITE_STORAGES="../examples/storages/*.json"

#
# Build
cmake \
    -G "$GENERATOR" \
    -DCMAKE_INSTALL_PREFIX:PATH="$INSTALL_PREFIX" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DWITH_HDF5=$WITH_HDF5 \
    -DHDF5_DIR="$INSTALL_PREFIX/cmake/hdf5" \
    -DHDF5_ROOT="$INSTALL_PREFIX" \
    -DWITH_PYTHON=$WITH_PYTHON \
    -DSWIG_DIR="$SWIG_DIR" \
    -DSWIG_EXECUTABLE="$SWIG_EXECUTABLE" \
    ..
cmake --build . --config $config

# Build in Debug mode doesn't work since we then will try to link to
# python37_d.lib which doesn't exists
#cmake --build . --config Debug


#
# Tests
#
ctest

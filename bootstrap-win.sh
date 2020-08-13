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
CMAKE_PATH="/c/Program Files/CMake/bin/cmake"
SWIG_PATH="$HOME/Documents/Software/swigwin-4.0.1"
GENERATOR="Visual Studio 15 2017 Win64"
HDF5_VERSION="1.10.4"
PYTHON_EXECUTABLE="/c/Program Files (x86)/Microsoft Visual Studio/Shared/Anaconda3_64/python"
#
# Init environment variables
#
ROOT_PATH=$(realpath $(pwd)/..)
PATH=$PATH:$CMAKE_PATH:$SWIG_PATH:$ROOT_PATH/local/bin
INSTALL_PREFIX=$ROOT_PATH/local
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
# Build and install jansson
#
if [ ! -f $INSTALL_PREFIX/lib/jansson.lib ]; then
    cd "$ROOT_PATH"
    if [ ! -d jansson ]; then
        git clone https://github.com/akheron/jansson.git
    fi
    cd jansson
    mkdir -p build && cd build
    cmake \
        -G "$GENERATOR" \
        -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_PREFIX \
        -DJANSSON_BUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DJANSSON_BUILD_DOCS=OFF \
        ..
    cmake --build . --config Debug --target install
    cmake --build . --config Release --target install
fi
#
# Build and install hdf5
#
if [ ! -f $INSTALL_PREFIX/lib/libhdf5.lib ]; then
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
    mkdir -p build && cd build
    cmake \
        -G "$GENERATOR" \
        -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_PREFIX \
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
mkdir -p build && cd build
cmake \
    -G "$GENERATOR" \
    -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_PREFIX \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DHDF5_DIR=$INSTALL_PREFIX/cmake/hdf5 \
    -DHDF5_ROOT=$INSTALL_PREFIX
    ..
cmake --build . --config Release



# Build in Debug mode doesn't work since we then will try to link to
# python37_d.lib which doesn't exists
#cmake --build . --config Debug

#!/bin/sh
#
# Check and modify the path of the following variables
#
#CMAKE="/c/Program Files (x86)/Microsoft Visual Studio/2017/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
CMAKE=cmake
HDF5_ROOT="C:/Program Files/HDF_Group/HDF5/1.10.4"
JANSSON_ROOT="${USERPROFILE}/Documents/Software/jansson"
INSTALL_PREFIX="${USERPROFILE}/Documents/Software"
rootdir="$PWD"

set -ex
export HDF5_DIR="$HDF5_ROOT/cmake"


# -- uuid
cd "$rootdir"
mkdir -p uuid/build
cd uuid/build
"$CMAKE" \
    -G 'Visual Studio 15 2017 Win64' \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}/uuid" \
    ..
"$CMAKE" --build . --config Debug
"$CMAKE" --build . --config Debug --target test
"$CMAKE" --build . --config Debug --target install



# -- dlite
cd "$rootdir"
mkdir -p build
cd build
"$CMAKE" \
    -G 'Visual Studio 15 2017 Win64' \
    -WITH_HDF5=ON \
    -DHDF5_ROOT="$HDF5_ROOT/cmake/hdf5" \
    -DJANSSON_ROOT="$JANSSON_ROOT" \
    -DUUID_ROOT="$INSTALL_PREFIX/uuid" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}/dlite" \
    ..
"$CMAKE" --build . --config Debug
#"$CMAKE" --build . --config Debug --target test
#"$CMAKE" --build . --config Debug --target install

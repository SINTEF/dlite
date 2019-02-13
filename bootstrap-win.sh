#
# Check and modify the path of the following variables
#
CMAKE_PATH=/C/DREAM3D/SDK/cmake-3.11.2-win64-x64/bin
PYTHON_EXECUTABLE=/C/Users/tco/AppData/Local/Continuum/Anaconda3/python.exe
ROOT_PATH=/C/Users/tco/Documents/Programs/philib
GENERATOR="Visual Studio 14 2015 Win64"
#
# Check and modify the version of HDF5
#
HDF5_VERSION_STRING="1.10.4"
#
# Init environment variables
#
PATH=$PATH:$CMAKE_PATH:$ROOT_PATH/local/bin
LIB_INSTALL_PATH=$ROOT_PATH/local
#
# Build and install jansson
#
cd $ROOT_PATH
git clone https://github.com/akheron/jansson.git
cd jansson
mkdir build && cd build
cmake \
    -G "$GENERATOR" \
    -DCMAKE_INSTALL_PREFIX:PATH=$LIB_INSTALL_PATH \
    -DJANSSON_BUILD_SHARED_LIBS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DJANSSON_BUILD_DOCS=OFF \
    ..
cmake --build . --config Debug --target install
cmake --build . --config Release --target install
#
# Build and install hdf5
#
cd $ROOT_PATH
tar -xzvf hdf5-$HDF5_VERSION_STRING.tar.gz
cd hdf5-$HDF5_VERSION_STRING
mkdir build && cd build
cmake \
    -G "$GENERATOR" \
    -DCMAKE_INSTALL_PREFIX:PATH=$LIB_INSTALL_PATH \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    ..
cmake --build . --config Debug --target install
cmake --build . --config Release --target install
#
# Configure and generate dlite project
#
cd $ROOT_PATH/dlite
git submodule update --init
mkdir build && cd build
cmake \
    -G "$GENERATOR" \
    -DHDF5_DIR=$LIB_INSTALL_PATH/cmake/hdf5 \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_INSTALL_PREFIX:PATH=$LIB_INSTALL_PATH \
    ..
cmake --build . --config Debug
cmake --build . --config Release

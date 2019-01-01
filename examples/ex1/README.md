dlite -- example 1
=============================================================

Build an example to use dlite with C language.

## Build and run on Microsoft Windows

1. Install dlite
2. Open a git-bash window:
	1. Change the directory to the `ex1` folder
	2. Set environment variable LIB_PATH to the installation directory of hdf5, jansson and dlite.
	3. Run the commands below

```sh
LIB_PATH=/C/Users/tco/Documents/Programs/philib/local
PATH=$PATH:$LIB_PATH/bin
mkdir build && cd build
cmake -G "Visual Studio 14 2015 Win64" -DHDF5_DIR=$LIB_PATH/cmake/hdf5 -DJANSSON_ROOT=$LIB_PATH -DDLITE_ROOT=$LIB_PATH ..
cmake --build . --config Debug
./Debug/dlite-example-1.exe
```

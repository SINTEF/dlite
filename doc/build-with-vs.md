Build with Visual Studio
========================

1. Install a recent version of [CMake][cmake]
2. Install Visual Studio 14 2015 or Visual Studio 15 2017 with the
   C/C++ components

3. Prepare a directory structure for DLite and 3rd party libraries:

     1. Select a root folder to create the directory structure. Ex:

            "C:\Users\{username}\Documents\"

     2. Create the following directory:

            {root}\local

     3. Clone the DLite repository in the root folder, creating the folder

            {root}\dlite

4. Download hdf5 library archive from
   [https://support.hdfgroup.org/ftp/HDF5/current/src/][hdf5]

5. Copy of the file

            {root}\dlite\bootstrap-win.sh

   into the root folder and edit it as follows:

	1. Modify the variable CMAKE_PATH to the path of cmake directory
	2. Modify the variable ROOT_PATH to your root folder
	3. Modify the variable PYTHON_EXECUTABLE to Python version 3.x
	   executable
	4. Check the version number of hdf5 library
    5. To build in Win64 mode, add '-G "Visual Studio 15 Win64"' after
       cmake command when generating the solution (not building)

6. Open a git-bash window:
	1. Change the directory to your root folder
	2. Type `sh bootstrap-win.sh` and press enter

7. Open the file

            {root}\dlite\build\dlite.sln

   in Visual Studio

8. In Visual Studio:

    1. Select the solution configuration "Debug", then build the
       solution (Menu Build -> Build solution)
    2. Select the solution configuration "Release", then build the
       solution (Menu Build -> Build solution)

To run the tests, do

    ctest -C Debug


Summary to build and install DLite when hdf5 lib are installed
in the given path LOCAL_DIR

    LOCAL_DIR=/C/Users/tco/Documents/Programs/philib/local
    PATH=$PATH:$LOCAL_DIR/bin
    cd dlite
    mkdir build && cd build
    cmake -G "Visual Studio 14 2015 Win64" -DHDF5_DIR=$LOCAL_DIR/cmake/hdf5 -DCMAKE_INSTALL_PREFIX=$LOCAL_DIR/ ..
    cmake --build . --config Debug --target doc
    cmake --build . --config Debug --target install
    cmake --build . --config Release --target install
    ctest -C Debug
    ctest -C Release


[cmake]: https://cmake.org/download/
[hdf5]: https://support.hdfgroup.org/ftp/HDF5/current/src/

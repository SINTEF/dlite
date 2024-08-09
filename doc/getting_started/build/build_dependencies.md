Installing build dependencies
=============================
For a basic build, at least the following is required:
- C compiler and standard C development libraries (ex: `sudo apt install build-essential`)
- [CMake] (ex `sudo apt install cmake`)
- [git] (ex `sudo apt install git`)

For configuring [CMake], the `ccmake` tool is handy.  Install it with `sudo apt install cmake-curses-gui`.  Run it as `ccmake .` from the build directory.


Python bindings (optional)
--------------------------
For Python bindings, you need Python with development libraries, [SWIG] and [NumPy] installed.
The following example shows how to install Python 3.10 on Ubuntu (which currently comes with Python 3.8) together with development libraries:

    sudo add-apt-repository ppa:deadsnakes/ppa -y
    sudo apt install python3.10 python3.10-dev python3.10-distutils swig
    pip install numpy  # you might want to do this from a virtual environemnt


Fortran bindings (optional)
---------------------------
For Fortran bindings, you need Fortran development libraries.
On Ubuntu, they can be installed with

    sudo apt install gfortran


Redland librdf support (optional)
---------------------------------
Redland [librdf] is a set of C libraries providing support for RDF.
It is currently used by the rdf storage plugin.

    sudo apt install librdf-dev


HDF5 support (optional)
-----------------------
[HDF5] development development libraries are used by the hdf5 storage plugin.
On Ubuntu, they can be installed with

    sudo apt install libhdf5-dev


Testing (optional)
------------------
DLite uses [cppcheck] for static code analysis and [valgrind] for memory checking.
On Ubuntu they can be installed with

    sudo apt install cppcheck valgrind


Debugging (optional)
--------------------
Use [gdb] for debugging errors in the C code.  On Ubuntu it can be installed with

    sudo apt install gdb

See the [GDB Tutorial] for more information on how to use GDB.  Some DLite-specific tips can also be found in [Memory debugging].


Building documentation (optional)
---------------------------------
DLite uses [Sphinx] and [Doxygen] to generate documentation.
Ubuntu they can be installed with

    sudo apt install cppcheck doxygen
    pip install -r requirements_doc.txt


[git]: https://git-scm.com/
[CMake]: https://cmake.org/
[NumPy]: https://pypi.org/project/numpy/
[SWIG]: https://www.swig.org/
[HDF5]: https://support.hdfgroup.org/HDF5/
[librdf]: https://librdf.org/
[cppcheck]: http://cppcheck.sourceforge.net/
[Doxygen]: http://www.doxygen.org/
[Sphinx]: https://www.sphinx-doc.org/
[valgrind]: http://valgrind.org/
[gdb]: https://sourceware.org/gdb/
[GDB Tutorial]: https://www.gdbtutorial.com/
[Memory debugging]: https://sintef.github.io/dlite/contributors_guidetips_and_tricks.html#memory-debugging

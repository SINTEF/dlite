Tips and Tricks
===============

Setting up a virtual Python environment for building dlite
----------------------------------------------------------
With [virtualenvwrapper], do

    pip install virtualenvwrapper
    export WORKON_HOME=$HOME/.envs  # Put this in your ~/.bash_profile
    mkvirtualenv dlite
    pip install -U pip
    pip install -r requirements.txt
    pip install -r requirements_dev.txt
    pip install -r requirements_doc.txt

    workon dlite  #  only needed if you are not already in your dlite virtualenv
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=$WORKON_HOME/dlite ..
    make
    make install


Debugging tests failing inside docker on GitHub
-----------------------------------------------
First you can try to set up a local virtual environment with the same
version of Python as in the failing test.


### Setting up a local virtual environment with given version of python
See the documentation in the build-python3.7.cmake file included with
dlite.


### Debugging within the docker environment
First, enter your virtual environment with dlite and install cibuildwheel.

    workon dlite
    pip install cibuildwheel

To list all manylinux images for Python 3.7, do

    CIBW_MANYLINUX_X86_64_IMAGE=ghcr.io/sintef/dlite-python-manylinux2014_x86_64:latest \
    CIBW_BUILD=cp37-manylinux_* \
    python -m cibuildwheel \
    --print-build-identifiers \
    --platform linux \
    python

This should write

    cp37-manylinux_x86_64
    cp37-manylinux_i686

To run the image `cp37-manylinux_x86_64` do

    CIBW_MANYLINUX_X86_64_IMAGE=ghcr.io/sintef/dlite-python-manylinux2014_x86_64:latest \
    CIBW_BUILD=cp37-manylinux_x86* \
    python -m cibuildwheel \
    --output-dir wheelhouse \
    --platform linux \
    python

which should run the tests and hopefully fail at the same place as on
GitHub.  If that is the case, you can run the image again, but pause
it before running the tests by adding prepending
`CIBW_BEFORE_TEST=cat` to the previous command:

    CIBW_BEFORE_TEST=cat \
    CIBW_MANYLINUX_X86_64_IMAGE=ghcr.io/sintef/dlite-python-manylinux2014_x86_64:latest \
    CIBW_BUILD=cp37-manylinux_x86* \
    python -m cibuildwheel \
    --output-dir wheelhouse \
    --platform linux \
    python


Now you can list all active containers with

    docker ps -a

the first container should have a name starting with `cibuildwheel-`.
Copy its id and run

    ID=91312b22090f  # replace this id with the one listed by docker ps -a
    docker exec -it $ID /bin/sh

This should start a new shell inside the container that the test was run it.

    cd /project/build



[virtualenvwrapper]: https://pypi.org/project/virtualenvwrapper/

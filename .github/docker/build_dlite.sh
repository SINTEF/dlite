#!/bin/bash
set -eux


function repair_wheel {
    wheel="$1"
    if ! auditwheel show "$wheel"; then
        echo "Skipping non-platform wheel $wheel"
    else
        auditwheel repair "$wheel" --plat "$PLAT" -w /io/wheelhouse/
    fi
}




    export Python3_EXECUTABLE=/opt/python/${ABITAG}/bin/python${PYVER}
    export Python3_LIBRARY=/opt/python/${ABITAG}/lib/libpython${PYVER}.a
    export Python3_INCLUDE_DIR=/opt/python/${ABITAG}/include/python${PYVER}




# Outdated hdf5 library in Centos
mkdir -p $BUILDDIR
cd $BUILDDIR
$CMAKE \
    -DCMAKE_INSTALL_PREFIX=$INSTALLDIR \
    -DPython3_EXECUTABLE=$Python3_EXECUTABLE \
    -DPython3_LIBRARY=$Python3_LIBRARY \
    -DPython3_INCLUDE_DIR=$Python3_INCLUDE_DIR \
    -DWITH_HDF5=NO \
    -DWITH_DOC=NO \
    -DWITH_STATIC_PYTHON=YES \
    ../../..

#    -Dlink_libraries_extra=$Python3_LIBRARY \

$CMAKE --build . --config Release



    # Compile wheel
    set -x
    ${Python3_EXECUTABLE} -m pip wheel /io/python --no-deps -w wheelhouse/

    # Bundle external shared libraries into the wheels
    repair_wheel wheelhouse/DLite_Python-*-${ABITAG}-*.whl

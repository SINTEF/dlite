#!/usr/bin/env bash
# Run cibuildwheel
#
# Usage:
#
#       run_cibuildwheel.sh CIBW_BUILD
#
# where CIBW_BUILD is the value for the `CIBW_BUILD` env variable for `cibuildwheel`.
# More information: https://cibuildwheel.readthedocs.io/en/stable/options/#build-skip
set -eu

if [[ $# -eq 0 ]]; then
    echo "Please define a 'CIBW_BUILD' input."
    exit 1
else
    CIBW_BUILD=$1
fi

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILD_IDS_CMD="cibuildwheel --platform linux --print-build-identifiers ${SCRIPT_DIR}/../../python"

for identifier in $(CIBW_BUILD=${CIBW_BUILD} ${BUILD_IDS_CMD}); do
    SYSTEM_TYPE=$( echo ${identifier} | sed -e 's|^.*-||' | sed -e 's|_.*$||' )
    ARCH=$( echo ${identifier} | sed -e 's|^.*linux_||' )
    echo "SYSTEM=${SYSTEM_TYPE} and ARCH=${ARCH} for '${identifier}'"
done


# CIBW_MANYLINUX_X86_64_IMAGE=dlite-python-manylinux${{ matrix.system_type[1] }}_x86_64:latest
# CIBW_MANYLINUX_I686_IMAGE=dlite-python-manylinux${{ matrix.system_type[1] }}_i686:latest
CIBW_MUSLLINUX_X86_64_IMAGE=dlite-python-musllinux_1_1_x86_64:latest
CIBW_MUSLLINUX_I686_IMAGE=dlite-python-musllinux_1_1_i686:latest

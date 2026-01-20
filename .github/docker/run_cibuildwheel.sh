#!/bin/bash
set -eu

DESCR='Run cibuildwheel for given {SYSTEM}, {TYPE} and {ARCH}.'

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)"

# Command to execute
command() {
    for minor in $PY_MINORS; do

        CIBW_BUILD=cp3${minor}-${SYSTEM}_${ARCH}

        CIBW_MANYLINUX_X86_64_IMAGE=dlite-python-manylinux${TYPE}_x86_64:latest
        CIBW_MANYLINUX_I686_IMAGE=dlite-python-manylinux${TYPE}_i686:latest
        CIBW_MUSLLINUX_X86_64_IMAGE=dlite-python-musllinux${TYPE}_x86_64:latest
        CIBW_MUSLLINUX_I686_IMAGE=dlite-python-musllinux${TYPE}_i686:latest

        ROOT_DIR="$SCRIPT_DIR"/../..

        BUILD_IDS_CMD="cibuildwheel --platform linux --print-build-identifiers ${ROOT_DIR}/python"

        FINISHED_SYSTEM=""
        mkdir -p /tmp/docker_build_wheel
        for identifier in $(CIBW_BUILD=${CIBW_BUILD} ${BUILD_IDS_CMD}); do
            #SYSTEM=$( echo ${identifier} | sed -e 's|^.*-||' | sed -E -e 's/_(i686|x86_64)$//' )
            #ARCH=$( echo ${identifier} | sed -e 's|^.*linux_||' )

            #echo "SYSTEM=${SYSTEM}, TYPE=${TYPE}, ARCH=${ARCH} for '${identifier}'"

            if [ -n "${FINISHED_SYSTEM}" ] && [[ "${FINISHED_SYSTEM}" == *"${SYSTEM}_${ARCH}"* ]]; then
                # echo "Already built ${SYSTEM}_${ARCH} image - skipping."
                continue
            fi

            ${SCRIPT_DIR}/gen_dockerfile.sh \
                -s $SYSTEM -t $TYPE -a $ARCH -o /tmp/docker_build_wheel


            echo "Build Docker image for ${SYSTEM}${TYPE}_${ARCH}"

            docker build \
                -t dlite-python-${SYSTEM}${TYPE}_${ARCH}:latest \
                -f /tmp/docker_build_wheel/Dockerfile-${SYSTEM}${TYPE}_${ARCH} \
                .

            FINISHED_SYSTEM=${FINISHED_SYSTEM}:${SYSTEM}_${ARCH}
        done

        rm -rf /tmp/docker_build_wheel

        # Run cibuildwheel
        CIBW_BUILD=${CIBW_BUILD} \
        CIBW_MANYLINUX_X86_64_IMAGE=${CIBW_MANYLINUX_X86_64_IMAGE} \
        CIBW_MANYLINUX_I686_IMAGE=${CIBW_MANYLINUX_I686_IMAGE} \
        CIBW_MUSLLINUX_X86_64_IMAGE=${CIBW_MUSLLINUX_X86_64_IMAGE} \
        CIBW_MUSLLINUX_I686_IMAGE=${CIBW_MUSLLINUX_I686_IMAGE} \
        cibuildwheel \
            --platform linux \
            --output-dir ${ROOT_DIR}/python/wheelhouse \
            ${ROOT_DIR}/python

}

source "$SCRIPT_DIR"/command.sh



exit



# Run cibuildwheel
#
# Usage:
#
#     run_cibuildwheel.sh CIBW_BUILD SYSTEM_TYPE
#
# where CIBW_BUILD is the value for the `CIBW_BUILD` env variable for `cibuildwheel`
# and SYSTEM_TYPE is the specific linux system type, e.g., 2010 or 2014 for manylinux
# and _1_1 or _1_2 for musllinux
# More information: https://cibuildwheel.readthedocs.io/en/stable/options/#build-skip
#
# Example:
#
#     run_cibuildwheel.sh cp38-musllinux_x86_64 _1_1
set -eu

if [[ $# -eq 0 ]]; then
    echo "Please define a 'CIBW_BUILD' and 'SYSTEM_TYPE' input."
    exit 1
elif [[ $# -eq 1 ]]; then
    echo "Please also define 'SYSTEM_TYPE' input."
    exit 1
fi
CIBW_BUILD=$1
SYSTEM_TYPE=$2

CIBW_MANYLINUX_X86_64_IMAGE=dlite-python-manylinux${SYSTEM_TYPE}_x86_64:latest
CIBW_MANYLINUX_I686_IMAGE=dlite-python-manylinux${SYSTEM_TYPE}_i686:latest
CIBW_MUSLLINUX_X86_64_IMAGE=dlite-python-musllinux${SYSTEM_TYPE}_x86_64:latest
CIBW_MUSLLINUX_I686_IMAGE=dlite-python-musllinux${SYSTEM_TYPE}_i686:latest

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ROOT_DIR=$SCRIPT_DIR/../..

BUILD_IDS_CMD="cibuildwheel --platform linux --print-build-identifiers ${ROOT_DIR}/python"

FINISHED_SYSTEM=""
mkdir -p /tmp/docker_build_wheel
for identifier in $(CIBW_BUILD=${CIBW_BUILD} ${BUILD_IDS_CMD}); do
    SYSTEM=$( echo ${identifier} | sed -e 's|^.*-||' | sed -E -e 's/_(i686|x86_64)$//' )
    ARCH=$( echo ${identifier} | sed -e 's|^.*linux_||' )

    #echo "SYSTEM=${SYSTEM}, SYSTEM_TYPE=${SYSTEM_TYPE}, ARCH=${ARCH} for '${identifier}'"

    if [ -n "${FINISHED_SYSTEM}" ] && [[ "${FINISHED_SYSTEM}" == *"${SYSTEM}_${ARCH}"* ]]; then
        # echo "Already built ${SYSTEM}_${ARCH} image - skipping."
        continue
    fi

    bash ${SCRIPT_DIR}/gen_dockerfile.sh $SYSTEM $SYSTEM_TYPE $ARCH \
         >/tmp/docker_build_wheel/Dockerfile-${SYSTEM}${SYSTEM_TYPE}_${ARCH}

    echo "Build Docker image for ${SYSTEM}${SYSTEM_TYPE}_${ARCH}"

    docker build \
        -t dlite-python-${SYSTEM}${SYSTEM_TYPE}_${ARCH}:latest \
        -f /tmp/docker_build_wheel/Dockerfile-${SYSTEM}${SYSTEM_TYPE}_${ARCH} \
        .

    FINISHED_SYSTEM=${FINISHED_SYSTEM}:${SYSTEM}_${ARCH}
done

rm -rf /tmp/docker_build_wheel

# Run cibuildwheel
CIBW_BUILD=${CIBW_BUILD} \
    CIBW_MANYLINUX_X86_64_IMAGE=${CIBW_MANYLINUX_X86_64_IMAGE} \
    CIBW_MANYLINUX_I686_IMAGE=${CIBW_MANYLINUX_I686_IMAGE} \
    CIBW_MUSLLINUX_X86_64_IMAGE=${CIBW_MUSLLINUX_X86_64_IMAGE} \
    CIBW_MUSLLINUX_I686_IMAGE=${CIBW_MUSLLINUX_I686_IMAGE} \
    cibuildwheel --platform linux --output-dir ${ROOT_DIR}/python/wheelhouse ${ROOT_DIR}/python

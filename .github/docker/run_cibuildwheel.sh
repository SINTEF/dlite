#!/usr/bin/env bash
# Run cibuildwheel
#
# Usage:
#
#       run_cibuildwheel.sh CIBW_BUILD SYSTEM_TYPE...
#
# where CIBW_BUILD is the value for the `CIBW_BUILD` env variable for `cibuildwheel`
# and SYSTEM_TYPE is the specific linux system type, e.g., 2010 or 2014 for manylinux
# and _1_1 for musllinux
# More information: https://cibuildwheel.readthedocs.io/en/stable/options/#build-skip
set -eu

if [[ $# -eq 0 ]]; then
    echo "Please define a 'CIBW_BUILD' and at least one 'SYSTEM_TYPE' input."
    exit 1
elif [[ $# -eq 1 ]]; then
    echo "Please also define at least one 'SYSTEM_TYPE' input."
    exit 1
fi
CIBW_BUILD=$1

GIVEN_TYPE=""
shift  # Skip first input
for i in "$@"; do
    GIVEN_TYPE=${GIVEN_TYPE}:${i}
done

if [[ "${GIVEN_TYPE}" == *":"* ]] && [[ "${GIVEN_TYPE}" == *"2010"* ]] && [[ "${GIVEN_TYPE}" == *"2014"* ]]; then
    echo "You cannot pass both '2010' and '2014' as 'SYSTEM_TYPE's."
    exit 1
fi
if [[ "${GIVEN_TYPE}" == *"2010"* ]]; then
    MANY_LINUX_TYPE=2010
elif  [[ "${GIVEN_TYPE}" == *"2014"* ]]; then
    MANY_LINUX_TYPE=2014
fi

CIBW_MANYLINUX_X86_64_IMAGE=dlite-python-manylinux${MANY_LINUX_TYPE}_x86_64:latest
CIBW_MANYLINUX_I686_IMAGE=dlite-python-manylinux${MANY_LINUX_TYPE}_i686:latest
CIBW_MUSLLINUX_X86_64_IMAGE=dlite-python-musllinux_1_1_x86_64:latest
CIBW_MUSLLINUX_I686_IMAGE=dlite-python-musllinux_1_1_i686:latest

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILD_IDS_CMD="cibuildwheel --platform linux --print-build-identifiers ${SCRIPT_DIR}/../../python"

FINISHED_SYSTEM=""
mkdir -p /tmp/docker_build_wheel
for identifier in $(CIBW_BUILD=${CIBW_BUILD} ${BUILD_IDS_CMD}); do
    SYSTEM=$( echo ${identifier} | sed -e 's|^.*-||' | sed -E -e 's/_(i686|x86_64)$//' )
    ARCH=$( echo ${identifier} | sed -e 's|^.*linux_||' )
    # echo "SYSTEM=${SYSTEM} and ARCH=${ARCH} for '${identifier}'"

    if [ "${SYSTEM}" == "manylinux" ]; then
        SYSTEM_TYPE=${MANY_LINUX_TYPE}
    elif [ "${SYSTEM}" == "musllinux" ]; then
        SYSTEM_TYPE=_1_1
    else
        echo "This script is only valid for linux builds ('${identifier}')."
        exit 1
    fi

    if [ -n "${FINISHED_SYSTEM}" ] && [[ "${FINISHED_SYSTEM}" == *"${SYSTEM}_${ARCH}"* ]]; then
        # echo "Already built ${SYSTEM}_${ARCH} image - skipping."
        continue
    fi
    echo "Will build Docker image for ${SYSTEM}${SYSTEM_TYPE}_${ARCH}"

    sed \
        -e "s|{{ ARCH }}|${ARCH}|" \
        -e "s|{{ TYPE }}|${SYSTEM_TYPE}|" \
        .github/docker/Dockerfile-${SYSTEM}.template \
        > /tmp/docker_build_wheel/Dockerfile-${SYSTEM}${SYSTEM_TYPE}_${ARCH}

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
    cibuildwheel --platform linux ${SCRIPT_DIR}/../../python

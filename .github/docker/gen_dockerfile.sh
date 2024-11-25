#!/bin/bash
# Write Dockerfile for given system to stdout.
#
# Usage:
#
#     gen_dockerfile.sh SYSTEM SYSTEM_TYPE ARCH
#
# Arguments:
#   SYSTEM: system type. Ex: manylinux, musllinux
#   SYSTEM_TYPE: Ex: 2010, 2014, _2_24, _2_28 (manylinux), _1_1, _1_2 (musllinux)
#   ARCH: Ex: x86_64, i686
set -eu

if [[ $# -ne 3 ]]; then
    echo "Usage: gen_dockerfile.sh SYSTEM SYSTEM_TYPE ARCH"
    exit 1
fi
SYSTEM=$1
SYSTEM_TYPE=$2
ARCH=$3


SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd -P )
REL_DIR=${SCRIPT_DIR##*/dlite/}


if [ $SYSTEM = "manylinux" -a ${SYSTEM_TYPE:0:1} = "_" ]; then
    template=${REL_DIR}/Dockerfile-${SYSTEM}_x_y.template
else
    template=${REL_DIR}/Dockerfile-${SYSTEM}.template
fi

EXTRA_PRE=""
EXTRA_POST=""
if [ ${SYSTEM_TYPE} == "2010" ]; then
    EXTRA_PRE="COPY ${REL_DIR}/pgdg-91_${ARCH}.repo /etc/yum.repos.d/pgdg-91.repo"
    EXTRA_POST="ENV PATH=\$PATH:/usr/pgsql-9.1/bin"
fi

sed \
    -e "s|{{ ARCH }}|${ARCH}|" \
    -e "s|{{ TYPE }}|${SYSTEM_TYPE}|" \
    -e "s|{{ EXTRA_PRE }}|${EXTRA_PRE}|" \
    -e "s|{{ EXTRA_POST }}|${EXTRA_POST}|" \
    $template

#!/bin/bash
#
# Usage: mkdockerfile.sh DISTRIBUTION BASE_IMAGE PY_MINORS
# Write dockerfile to stdout.
set -eu

DISTRIBUTION=$1
BASE_IMAGE=$2
PY_MINORS=$3

THISDIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)"

set -x
sed \
    -e "s|{{ BASE_IMAGE }}|${BASE_IMAGE}|" \
    -e "s|{{ PY_MINORS }}|${PY_MINORS}|" \
    $THISDIR/Dockerfile-$DISTRIBUTION

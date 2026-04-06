#!/bin/bash
#
# Usage: mkdockerfile.sh DISTRIBUTION BASE_IMAGE PY_MINORS
# Write dockerfile to stdout.
set -eu

DISTRIBUTION=$1
BASE_IMAGE=$2
PY_MINORS=$3

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)"

sed \
    -e "s|{{ BASE_IMAGE }}|${BASE_IMAGE}|" \
    -e "s|{{ PY_MINORS }}|${PY_MINORS}|" \
    Dockerfile-$DISTRIBUTION

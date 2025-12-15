#!/bin/bash
set -eu

DESCR='Generate dockerfile(s) "Dockerfile-{SYSTEM}{TYPE}_{ARCH}" in {outdir}.'

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)"


# Command to execute
command() {
    outfile="Dockerfile-${SYSTEM}${TYPE}_${ARCH}"
    echo "Generate: $outfile"

    if [ $SYSTEM = "manylinux" -a ${TYPE:0:1} = "_" ]; then
        template=${SCRIPT_DIR}/Dockerfile-${SYSTEM}_x_y.template
    else
        template=${SCRIPT_DIR}/Dockerfile-${SYSTEM}.template
    fi

    EXTRA_PRE=""
    EXTRA_POST=""
    if [ ${TYPE} == "2010" ]; then
        EXTRA_PRE="COPY ${REL_DIR}/pgdg-91_${ARCH}.repo /etc/yum.repos.d/pgdg-91.repo"
        EXTRA_POST="ENV PATH=\$PATH:/usr/pgsql-9.1/bin"
    fi

    sed \
        -e "s|{{ ARCH }}|${ARCH}|" \
        -e "s|{{ TYPE }}|${TYPE}|" \
        -e "s|{{ PY_MINORS }}|${PY_MINORS}|" \
        -e "s|{{ EXTRA_PRE }}|${EXTRA_PRE}|" \
        -e "s|{{ EXTRA_POST }}|${EXTRA_POST}|" \
        $template >"$outdir"/"$outfile"
}

source "$SCRIPT_DIR"/command.sh

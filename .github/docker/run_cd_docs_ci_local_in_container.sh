#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="tmp"

cd /workspace

if [[ -n "${CI_SOURCE_REF:-}" ]]; then
  echo "== Source ref requested: ${CI_SOURCE_REF} =="
fi
if [[ -n "${CI_SOURCE_SHA:-}" ]]; then
  echo "== Expected source SHA: ${CI_SOURCE_SHA} =="
fi
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  ACTUAL_SHA="$(git rev-parse HEAD)"
  echo "== Container source SHA: ${ACTUAL_SHA} =="
  if [[ -n "${CI_SOURCE_SHA:-}" ]] && [[ "${ACTUAL_SHA}" != "${CI_SOURCE_SHA}" ]]; then
    echo "ERROR: Container source SHA does not match expected SHA."
    exit 1
  fi
fi

echo "== Tool versions =="
python --version
swig -version | head -n 1 || true
cmake --version | head -n 1
doxygen --version
dot -V

echo "== Build and verify docs (single source of truth) =="
if [[ ! -x tools/cd_docs_build_and_verify.sh ]]; then
  chmod +x tools/cd_docs_build_and_verify.sh
fi
BUILD_DIR="${BUILD_DIR}" tools/cd_docs_build_and_verify.sh

echo "== SUCCESS: Local cd_docs reproduction completed =="

#!/usr/bin/env bash
set -euo pipefail

# Single source of truth for docs CI build + AutoAPI verification.
BUILD_DIR="${BUILD_DIR:-tmp}"

echo "== Configure (cmake) =="
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
# Provide a writable home for cmake/python cache when running as an arbitrary UID
mkdir -p "${BUILD_DIR}/.home"
cd "${BUILD_DIR}"
Python3_ROOT="$(python3 -c 'import sys; print(sys.exec_prefix)')" \
  CFLAGS='-Wno-missing-field-initializers' \
  cmake .. -DFORCE_EXAMPLES=ON -DWITH_DOC=ON

echo "== Build (make) =="
make VERBOSE=1 2>&1 | tee make-docs.log

cd ..

echo "== Verify Python AutoAPI output =="
mapfile -t DOCS_CANDIDATES < <(find . -type f -path '*/doc/html/index.html' -not -path './gh-pages/*' | sed 's#/index.html$##' | sort -u)
if [[ "${#DOCS_CANDIDATES[@]}" -eq 0 ]]; then
  echo "ERROR: Could not locate Sphinx HTML output directory (*/doc/html)."
  echo "---- Candidate doc directories ----"
  find . -maxdepth 4 -type d -name doc | sort || true
  echo "---- Candidate html directories ----"
  find . -maxdepth 6 -type d -name html | sort || true
  exit 1
fi

DOCS_HTML_DIR=""
AUTOAPI_DIR=""
BEST_AUTOAPI_INDEX_COUNT=-1
for CANDIDATE_DIR in "${DOCS_CANDIDATES[@]}"; do
  CANDIDATE_AUTOAPI_DIR="${CANDIDATE_DIR}/autoapi"
  CANDIDATE_AUTOAPI_INDEX_COUNT=$(find "${CANDIDATE_AUTOAPI_DIR}" -type f -name index.html 2>/dev/null | wc -l)
  echo "Candidate docs HTML directory: ${CANDIDATE_DIR} (AutoAPI index pages: ${CANDIDATE_AUTOAPI_INDEX_COUNT})"
  if [[ "${CANDIDATE_AUTOAPI_INDEX_COUNT}" -gt "${BEST_AUTOAPI_INDEX_COUNT}" ]]; then
    BEST_AUTOAPI_INDEX_COUNT="${CANDIDATE_AUTOAPI_INDEX_COUNT}"
    DOCS_HTML_DIR="${CANDIDATE_DIR}"
    AUTOAPI_DIR="${CANDIDATE_AUTOAPI_DIR}"
  fi
done

echo "Using docs HTML directory: ${DOCS_HTML_DIR}"
if [[ -n "${GITHUB_ENV:-}" ]]; then
  echo "DOCS_HTML_DIR=${DOCS_HTML_DIR}" >> "${GITHUB_ENV}"
fi

if [[ "${BEST_AUTOAPI_INDEX_COUNT}" -lt 1 ]]; then
  echo "ERROR: AutoAPI output was not found in any docs HTML directory."
  echo "---- Make/Sphinx log tail ----"
  if [[ -f "${BUILD_DIR}/make-docs.log" ]]; then
    tail -n 300 "${BUILD_DIR}/make-docs.log" || true
  else
    echo "No ${BUILD_DIR}/make-docs.log found."
  fi
  echo "---- All AutoAPI directories under workspace ----"
  find . -type d -path '*/autoapi' | sort || true
  echo "---- All AutoAPI index files under workspace ----"
  find . -type f -path '*/autoapi/*/index.html' | sort | head -n 200 || true
  echo "---- Sphinx config excerpt ----"
  find . -type f -path '*/doc/_build/conf.py' -print -exec sed -n '1,260p' {} \; | grep -E 'autoapi_(dirs|ignore|file_patterns|python_use_implicit_namespaces)' || true
  echo "---- docs HTML listing ----"
  ls -la "${DOCS_HTML_DIR}" || true
  exit 1
fi

echo "Using AutoAPI directory: ${AUTOAPI_DIR}"
AUTOAPI_INDEX_COUNT="${BEST_AUTOAPI_INDEX_COUNT}"
echo "Detected ${AUTOAPI_INDEX_COUNT} AutoAPI index page(s)."

if [[ ! -f "${DOCS_HTML_DIR}/api.html" ]] || ! grep -Eq 'autoapi/.*/index.html' "${DOCS_HTML_DIR}/api.html"; then
  echo "ERROR: API landing page does not link to Python AutoAPI pages."
  echo "---- api.html excerpt ----"
  sed -n '460,620p' "${DOCS_HTML_DIR}/api.html" || true
  exit 1
fi

find "${AUTOAPI_DIR}" -maxdepth 3 -type f -name index.html | sort | head -n 20

echo "== SUCCESS: docs build + AutoAPI verification completed =="

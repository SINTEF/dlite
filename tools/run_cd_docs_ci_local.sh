#!/usr/bin/env bash
set -euo pipefail

IMAGE_NAME="dlite-cd-docs-local"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_REF="${1:-}"
WORKTREE_PATH=""
SNAPSHOT_PATH=""
EXPECTED_SHA=""
MOUNT_PATH=""
SOURCE_LABEL=""

cleanup() {
  local _exit=$?
  if [[ -n "${WORKTREE_PATH}" ]] && git -C "${REPO_ROOT}" worktree list --porcelain | grep -Fq "worktree ${WORKTREE_PATH}"; then
    git -C "${REPO_ROOT}" worktree remove --force "${WORKTREE_PATH}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${SNAPSHOT_PATH}" ]] && [[ -d "${SNAPSHOT_PATH}" ]]; then
    # Docker may have created root-owned build artifacts inside the snapshot;
    # use a throw-away container to remove them before falling back to rm -rf.
    docker run --rm \
      -v "${SNAPSHOT_PATH}:/toclean" \
      ubuntu:24.04 \
      rm -rf /toclean/tmp /toclean/build 2>/dev/null || true
    rm -rf "${SNAPSHOT_PATH}" 2>/dev/null || true
  fi
  exit "${_exit}"
}
trap cleanup EXIT

if [[ -n "${SOURCE_REF}" ]]; then
  EXPECTED_SHA="$(git -C "${REPO_ROOT}" rev-parse "${SOURCE_REF}")"
  WORKTREE_PATH="$(mktemp -d "${REPO_ROOT}/.cd-docs-local-worktree.XXXXXX")"
  rm -rf "${WORKTREE_PATH}"
  git -C "${REPO_ROOT}" worktree add --detach "${WORKTREE_PATH}" "${SOURCE_REF}" >/dev/null
  MOUNT_PATH="${WORKTREE_PATH}"
  SOURCE_LABEL="git ref ${SOURCE_REF} (${EXPECTED_SHA})"
else
  SNAPSHOT_PATH="$(mktemp -d "${REPO_ROOT}/.cd-docs-local-snapshot.XXXXXX")"
  rsync -a \
    --exclude='.git/' \
    --exclude='tmp/' \
    --exclude='build*/' \
    --exclude='ci-build/' \
    --exclude='dlite-builddliteenv/' \
    "${REPO_ROOT}/" "${SNAPSHOT_PATH}/"
  MOUNT_PATH="${SNAPSHOT_PATH}"
  SOURCE_LABEL="working-tree snapshot (tracked + untracked files)"
fi

echo "== Building local docs CI image (${IMAGE_NAME}) =="
docker build \
  -f "${REPO_ROOT}/.github/docker/Dockerfile-cd-docs-local" \
  -t "${IMAGE_NAME}" \
  "${REPO_ROOT}"

echo "== Running local docs CI repro from ${SOURCE_LABEL} =="
# --user ensures build artifacts are created with the host UID so cleanup
# (rm -rf) never hits "Permission denied" on root-owned Docker output.
# HOME=/workspace/tmp/.home gives cmake/python a writable home cache dir.
if [[ -n "${SOURCE_REF}" ]]; then
  docker run --rm -it \
    --user "$(id -u):$(id -g)" \
    -e HOME=/workspace/tmp/.home \
    -e "CI_SOURCE_REF=${SOURCE_REF}" \
    -e "CI_SOURCE_SHA=${EXPECTED_SHA}" \
    -v "${MOUNT_PATH}:/workspace" \
    -w /workspace \
    "${IMAGE_NAME}"
else
  docker run --rm -it \
    --user "$(id -u):$(id -g)" \
    -e HOME=/workspace/tmp/.home \
    -e "CI_SOURCE_REF=WORKTREE" \
    -v "${MOUNT_PATH}:/workspace" \
    -w /workspace \
    "${IMAGE_NAME}"
fi

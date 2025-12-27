#!/usr/bin/env bash
set -euo pipefail

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run these builds" >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

for distro in ubuntu-24.04 fedora-40; do
  docker build -f "${SCRIPT_DIR}/Dockerfile.${distro}" -t "vlc-xattr-plugin:${distro}" "${ROOT_DIR}"
done

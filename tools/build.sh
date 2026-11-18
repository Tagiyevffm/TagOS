#!/usr/bin/env bash
# TagOS build helper — configure, build and test the monorepo.
#
# Works on an Arch host (the supported platform) and on a Windows developer
# machine with MSYS2 UCRT64 + Qt6 (UI work only: no Wayland, no systemd).
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TAGOS_BUILD_DIR:-$ROOT/build/dev}"
BUILD_TYPE="${TAGOS_BUILD_TYPE:-RelWithDebInfo}"
GENERATOR="${TAGOS_GENERATOR:-Ninja}"
JOBS="${TAGOS_JOBS:-}"

usage() {
    cat <<'EOF'
usage: tools/build.sh [options]

  --build-dir <dir>     build directory (default build/dev)
  --type <type>         Debug | RelWithDebInfo | Release (default RelWithDebInfo)
  --clean               remove the build directory first
  --no-tests            skip ctest after building
  -h, --help            this help
EOF
}

run_tests=1
while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --type) BUILD_TYPE="$2"; shift 2 ;;
        --clean) rm -rf "$BUILD_DIR"; shift ;;
        --no-tests) run_tests=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage; exit 2 ;;
    esac
done

echo "== TagOS build: type=$BUILD_TYPE dir=$BUILD_DIR"
cmake -S "$ROOT" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX=/usr

if [[ -n "$JOBS" ]]; then
    cmake --build "$BUILD_DIR" --parallel "$JOBS"
else
    cmake --build "$BUILD_DIR"
fi

if [[ "$run_tests" -eq 1 ]]; then
    echo "== ctest"
    ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

echo "== done: $BUILD_DIR/bin"

#!/usr/bin/env bash
# build-arch.sh — build and test TagOS on Arch Linux, the authoritative platform.
#
# Arch Linux is the only platform where a build counts as a real TagOS build:
# Qt Wayland platform plugins, labwc, D-Bus and /proc are all present there.
# Windows builds are a development convenience and never proof of Linux support.
#
#   tools/build-arch.sh                 # configure + build + ctest
#   tools/build-arch.sh --deps          # install build dependencies first (sudo)
#   tools/build-arch.sh --clean         # wipe the build directory first
#   tools/build-arch.sh --prefix /tmp/x # staged install into a prefix
#   tools/build-arch.sh --install DIR   # DESTDIR install into DIR
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TAGOS_BUILD_DIR:-$ROOT/build/arch}"
BUILD_TYPE="${TAGOS_BUILD_TYPE:-RelWithDebInfo}"
INSTALL_DEPS=0
CLEAN=0
RUN_TESTS=1
DESTDIR_STAGE=""
PREFIX="/usr"

usage() {
    cat <<'EOF'
usage: tools/build-arch.sh [options]

  --build-dir <dir>   build directory (default build/arch)
  --type <type>       CMAKE_BUILD_TYPE (default RelWithDebInfo)
  --deps              install the build dependencies with pacman (root)
  --clean             remove the build directory first
  --no-tests          skip ctest
  --prefix <path>     install prefix recorded in the build (default /usr)
  --install <dir>     after tests, DESTDIR-install into <dir>
  -h, --help          this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --type) BUILD_TYPE="$2"; shift 2 ;;
        --deps) INSTALL_DEPS=1; shift ;;
        --clean) CLEAN=1; shift ;;
        --no-tests) RUN_TESTS=0; shift ;;
        --prefix) PREFIX="$2"; shift 2 ;;
        --install) DESTDIR_STAGE="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage; exit 2 ;;
    esac
done

if [[ "$(uname -s)" != "Linux" ]]; then
    cat >&2 <<'EOF'
build-arch.sh must run on Linux (Arch Linux recommended).
For the Linux-first workflow without a Linux machine use the CI job in
.github/workflows/ci.yml (archlinux container) and run tools/test-linux.sh there.
EOF
    exit 64
fi

DEPENDENCIES=(
    git cmake ninja gcc pkgconf python
    qt6-base qt6-declarative qt6-svg qt6-tools qt6-wayland
    # runtime dependencies are exercised by the smoke tests
    labwc pipewire wireplumber networkmanager bluez upower
    power-profiles-daemon polkit glib2 brightnessctl gammastep
)

if [[ "$INSTALL_DEPS" -eq 1 ]]; then
    if ! command -v pacman >/dev/null 2>&1; then
        echo "--deps only works on Arch Linux (pacman not found)" >&2
        exit 127
    fi
    echo "== installing build dependencies"
    sudo pacman -Syu --needed --noconfirm "${DEPENDENCIES[@]}"
fi

for tool in cmake ninja; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "missing $tool — run 'tools/build-arch.sh --deps' on Arch Linux" >&2
        exit 127
    fi
done

if [[ "$CLEAN" -eq 1 ]]; then
    echo "== removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

echo "== configuring ($BUILD_TYPE, prefix $PREFIX)"
cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DTAGOS_SYSCONF_DIR=/etc \
    -DTAGOS_BUILD_TESTS=ON

echo "== building"
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

if [[ "$RUN_TESTS" -eq 1 ]]; then
    echo "== testing"
    env QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software TAGOS_DRY_RUN=1 \
        ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

if [[ -n "$DESTDIR_STAGE" ]]; then
    echo "== installing into $DESTDIR_STAGE"
    rm -rf "$DESTDIR_STAGE"
    DESTDIR="$DESTDIR_STAGE" cmake --install "$BUILD_DIR" --prefix "$PREFIX"
    echo "== installed tree:"
    find "$DESTDIR_STAGE" -maxdepth 4 \( -name '*.service' -o -name '*.desktop' -o -name 'tag-*' \) \
        | sed "s|$DESTDIR_STAGE||" | sort | head -40
fi

cat <<EOF

TagOS build completed on Linux.
  build:   $BUILD_DIR
  tests:   $([[ "$RUN_TESTS" -eq 1 ]] && echo "ctest (headless)" || echo "skipped")
  next:    tools/package-arch.sh   # Arch package
           tools/build-iso.sh      # bootable ISO (root)
           tools/run-qemu.sh --iso out/tagos.iso
EOF

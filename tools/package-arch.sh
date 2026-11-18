#!/usr/bin/env bash
# package-arch.sh — build the TagOS Arch package (.pkg.tar.zst).
#
# Creates a reproducible source tarball from the current commit (git archive),
# then runs makepkg in a scratch directory so the repository never fills with
# build artefacts.
#
#   tools/package-arch.sh                 # build the package
#   tools/package-arch.sh --install-deps  # also install makedepends first (sudo)
#   tools/package-arch.sh --output <dir>  # default: out/pkg
#
# Must run on Arch Linux as a normal user: makepkg refuses to run as root.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${TAGOS_PKG_DIR:-$ROOT/out/pkg}"
INSTALL_DEPS=0

usage() {
    cat <<'EOF'
usage: tools/package-arch.sh [options]

  --output <dir>       package output directory (default out/pkg)
  --install-deps       install makedepends with pacman (sudo)
  -h, --help           this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output) OUT_DIR="$2"; shift 2 ;;
        --install-deps) INSTALL_DEPS=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage; exit 2 ;;
    esac
done

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "package-arch.sh requires Linux (Arch Linux for makepkg)." >&2
    exit 64
fi

if ! command -v makepkg >/dev/null 2>&1; then
    echo "makepkg not found — install 'pacman' (this script targets Arch Linux)." >&2
    exit 127
fi

if [[ "$(id -u)" -eq 0 ]]; then
    echo "makepkg must not run as root; run this script as your user." >&2
    exit 1
fi

if [[ "$INSTALL_DEPS" -eq 1 ]]; then
    # shellcheck disable=SC1091
    mapfile -t DEPS < <(bash -c 'source "$1"; printf "%s\n" "${makedepends[@]}"' _ "$ROOT/packaging/PKGBUILD")
    echo "== installing makedepends: ${DEPS[*]}"
    sudo pacman -S --needed --noconfirm "${DEPS[@]}"
fi

VERSION="$(sed -n 's/^pkgver=//p' "$ROOT/packaging/PKGBUILD")"
REL="$(sed -n 's/^pkgrel=//p' "$ROOT/packaging/PKGBUILD")"
WORK="$ROOT/build/pkg"
SRC_DIR="$WORK/src"
mkdir -p "$SRC_DIR" "$OUT_DIR"

if [[ -n "$(git -C "$ROOT" status --porcelain)" ]]; then
    echo "WARNING: working tree has uncommitted changes; the package is built from" >&2
    echo "         the last commit and will not contain them." >&2
fi

echo "== creating source tarball tagos-$VERSION.tar.gz from $(git -C "$ROOT" rev-parse --short HEAD)"
# git archive gives a clean, commit-identical tree: no build/ or out/ contents.
git -C "$ROOT" archive --format=tar.gz --prefix="tagos-$VERSION/" \
    -o "$SRC_DIR/tagos-$VERSION.tar.gz" HEAD

echo "== staging PKGBUILD"
rm -rf "$WORK/makepkg"
mkdir -p "$WORK/makepkg"
cp "$ROOT/packaging/PKGBUILD" "$ROOT/packaging/tagos.install" "$WORK/makepkg/"
cp "$SRC_DIR/tagos-$VERSION.tar.gz" "$WORK/makepkg/"

echo "== running makepkg"
pushd "$WORK/makepkg" >/dev/null
PKGDEST="$OUT_DIR" SRCDEST="$WORK/makepkg" makepkg --force --cleanbuild --noconfirm
popd >/dev/null

echo "== result"
shopt -s nullglob
packages=("$OUT_DIR"/tagos-"$VERSION"-"$REL"-*.pkg.tar.zst)
if [[ ${#packages[@]} -eq 0 ]]; then
    echo "makepkg produced no package in $OUT_DIR" >&2
    exit 1
fi
for pkg in "${packages[@]}"; do
    sha256sum "$pkg" | tee "$pkg.sha256"
    echo "== package contents (top level)"
    bsdtar -tf "$pkg" | awk -F/ '{print $1"/"$2}' | sort -u | head -20
done

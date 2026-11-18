#!/usr/bin/env bash
# test-linux.sh — run every Linux-side check for TagOS.
#
# This is the script a Linux CI runner executes. It refuses to run anywhere but
# Linux, because a green run on Windows says nothing about Wayland, D-Bus, /proc
# or the Arch package.
#
#   tools/test-linux.sh                # build dir default: build/arch
#   tools/test-linux.sh --build-dir build/ci
#   tools/test-linux.sh --with-dbus    # also exercise tag-flowd on a session bus
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TAGOS_BUILD_DIR:-$ROOT/build/arch}"
WITH_DBUS=0
WITH_LINT=1

usage() {
    cat <<'EOF'
usage: tools/test-linux.sh [options]

  --build-dir <dir>   existing build directory (default build/arch)
  --with-dbus         run tag-flowd against a private dbus-run-session
  --no-lint           skip qmllint / repository checks
  -h, --help          this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --with-dbus) WITH_DBUS=1; shift ;;
        --no-lint) WITH_LINT=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage; exit 2 ;;
    esac
done

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "test-linux.sh refuses to run on $(uname -s): Linux-only checks must run on Linux." >&2
    exit 64
fi

if [[ ! -x "$BUILD_DIR/bin/tag-shell" && ! -x "$BUILD_DIR/bin/tag-shell.exe" ]]; then
    echo "no build in $BUILD_DIR — run tools/build-arch.sh first" >&2
    exit 2
fi

BIN="$BUILD_DIR/bin"
failures=0
pass() { printf '  \033[32mPASS\033[0m %s\n' "$*"; }
fail() { printf '  \033[31mFAIL\033[0m %s\n' "$*"; failures=$((failures + 1)); }

echo "== platform"
echo "  kernel : $(uname -sr)"
# shellcheck source=/dev/null
echo "  distro : $(. /etc/os-release 2>/dev/null && echo "${PRETTY_NAME:-unknown}")"
echo "  session: ${XDG_SESSION_TYPE:-none} (compositor ${WAYLAND_DISPLAY:-none})"

echo "== ctest"
if env QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software TAGOS_DRY_RUN=1 \
        ctest --test-dir "$BUILD_DIR" --output-on-failure; then
    pass "ctest suite"
else
    fail "ctest suite"
fi

echo "== headless application start-up"
for app in tag-shell tag-files tag-settings tag-monitor tag-about; do
    if [[ ! -x "$BIN/$app" ]]; then
        fail "$app missing from the build"
        continue
    fi
    if env QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software TAGOS_DRY_RUN=1 \
            "$BIN/$app" --smoke-test >/tmp/"$app"-smoke.log 2>&1; then
        pass "$app --smoke-test"
    else
        fail "$app --smoke-test (see /tmp/$app-smoke.log)"
    fi
done

echo "== Linux system readers"
if "$BIN/tagos-metrics" >/tmp/tagos-metrics.log 2>&1; then
    pass "tagos-metrics reads the live system"
    sed -n '1,3p' /tmp/tagos-metrics.log | sed 's/^/    /'
else
    fail "tagos-metrics"
fi

if [[ -r /proc/self/stat && -r /proc/net/dev ]]; then
    pass "/proc is readable (process + network monitors have data)"
else
    fail "/proc missing — TagMonitor would have nothing to show"
fi

if [[ -d /sys/class/thermal ]] && compgen -G "/sys/class/thermal/thermal_zone*" >/dev/null; then
    pass "thermal zones present"
else
    echo "  SKIP thermal zones (no /sys/class/thermal on this machine)"
fi

if [[ "$WITH_DBUS" -eq 1 ]]; then
    echo "== D-Bus (private session bus)"
    if [[ ! -x "$BIN/tag-flowd" ]]; then
        echo "  SKIP tag-flowd not built (QtDBus missing)"
    elif dbus-run-session -- "$BIN/tag-flowd" --once --json >/tmp/flowd.json 2>/tmp/flowd.err; then
        if python3 -c "import json;d=json.load(open('/tmp/flowd.json'));assert d['schema']==1;assert d['memory']['totalBytes']>0"; then
            pass "tag-flowd produced a valid snapshot on the session bus"
        else
            fail "tag-flowd snapshot is not valid JSON/schema"
        fi
    else
        fail "tag-flowd (see /tmp/flowd.err)"
    fi
fi

if [[ "$WITH_LINT" -eq 1 && -x "$ROOT/tools/lint.sh" ]]; then
    echo "== lint"
    if TAGOS_BUILD_DIR="$BUILD_DIR" "$ROOT/tools/lint.sh" >/tmp/tagos-lint.log 2>&1; then
        pass "lint (qmllint, shellcheck, repository checks)"
    else
        fail "lint (see /tmp/tagos-lint.log)"
        tail -20 /tmp/tagos-lint.log | sed 's/^/    /'
    fi
fi

echo
if [[ "$failures" -eq 0 ]]; then
    echo "TAGOS_LINUX_TESTS_OK"
    exit 0
fi
echo "TAGOS_LINUX_TESTS_FAILED ($failures failing group(s))" >&2
exit 1

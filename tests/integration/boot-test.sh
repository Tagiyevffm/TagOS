#!/usr/bin/env bash
# End-to-end boot verification for a TagOS image.
#
# The image writes TAGOS_BOOT_OK to the serial console once a real user session
# reported readiness (tag-shell wrote /run/user/<uid>/tagos/ready). This script
# boots the image in QEMU with UEFI + serial and fails when either marker or the
# session is missing — it verifies more than "the kernel started".
#
#   tests/integration/boot-test.sh --iso out/tagos.iso
#   TAGOS_BOOT_TIMEOUT=300 tests/integration/boot-test.sh --iso out/tagos.iso
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
ISO=""
TIMEOUT="${TAGOS_BOOT_TIMEOUT:-240}"
LOG="${TAGOS_BOOT_LOG:-$ROOT/build/vm/boot-test.log}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --iso) ISO="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        --log) LOG="$2"; shift 2 ;;
        -h|--help) echo "usage: $0 --iso <path> [--timeout s] [--log file]"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

[[ -n "$ISO" && -f "$ISO" ]] || { echo "--iso <path> is required" >&2; exit 2; }

mkdir -p "$(dirname "$LOG")"
: >"$LOG"

echo "== booting $ISO headless (timeout ${TIMEOUT}s)"
set +e
TAGOS_BOOT_TIMEOUT="$TIMEOUT" \
    "$ROOT/tools/run-qemu.sh" --iso "$ISO" --boot-test --timeout "$TIMEOUT" --serial-log "$LOG"
status=$?
set -e

echo "== serial markers"
grep -E "TAGOS_BOOT_(WAITING|OK|TIMEOUT)" "$LOG" || echo "(none found)"

if [[ $status -ne 0 ]]; then
    echo "boot test FAILED" >&2
    exit $status
fi

# A session marker without a shell would mean the marker service lied: require
# that tag-shell actually announced itself.
if ! grep -q "tag-shell" "$LOG"; then
    echo "boot test FAILED: no tag-shell output on the serial console" >&2
    exit 1
fi

echo "boot test PASSED"

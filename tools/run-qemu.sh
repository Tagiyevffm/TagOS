#!/usr/bin/env bash
# TagOS development VM — QEMU with OVMF/UEFI, virtio devices and SPICE.
#
# Phase 0 hardware profile (see docs/phase-0.md):
#   3 GiB RAM · 4 vCPU · 32 GiB virtio disk · 1280x800 · Intel HDA · SPICE
#
# Examples:
#   tools/run-qemu.sh --create-disk            # first run
#   tools/run-qemu.sh                           # boot the installed system
#   tools/run-qemu.sh --iso out/tagos.iso       # boot the live medium
#   tools/run-qemu.sh --iso out/tagos.iso --boot-test
#                                               # headless serial boot check (CI)
set -euo pipefail
# QEMU device options are comma-separated inside one argument
# (-drive file=x,if=none,id=root): that is QEMU syntax, not an array separator.
# SC2054 is disabled repository-wide in .shellcheckrc for this reason.

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
DISK="${TAGOS_DISK:-$ROOT/build/vm/tagos.qcow2}"
ISO=""
MEMORY="${TAGOS_VM_MEMORY:-3G}"
CPUS="${TAGOS_VM_CPUS:-4}"
DISK_SIZE="${TAGOS_VM_DISK_SIZE:-32G}"
DISPLAY_MODE="spice"
GPU_MODE="plain"
BOOT_TEST=0
BOOT_TIMEOUT="${TAGOS_BOOT_TIMEOUT:-240}"
SERIAL_LOG=""
CREATE_DISK=0

usage() {
    cat <<'EOF'
usage: tools/run-qemu.sh [options]

  --iso <path>         boot the given ISO instead of the disk
  --disk <path>        disk image path (default build/vm/tagos.qcow2)
  --create-disk        create the disk image if it does not exist
  --memory <size>      RAM (default 3G)
  --cpus <n>           vCPUs (default 4)
  --display <mode>     spice | gtk | none (default spice)
  --gpu <mode>         plain | gl (default plain; gl needs host OpenGL)
  --serial-log <path>  write the serial console to a file
  --boot-test          headless boot; exit 0 only when TAGOS_BOOT_OK is seen
  --timeout <seconds>  boot-test timeout (default 240)
  -h, --help           this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --iso) ISO="$2"; shift 2 ;;
        --disk) DISK="$2"; shift 2 ;;
        --create-disk) CREATE_DISK=1; shift ;;
        --memory) MEMORY="$2"; shift 2 ;;
        --cpus) CPUS="$2"; shift 2 ;;
        --display) DISPLAY_MODE="$2"; shift 2 ;;
        --gpu) GPU_MODE="$2"; shift 2 ;;
        --serial-log) SERIAL_LOG="$2"; shift 2 ;;
        --boot-test) BOOT_TEST=1; DISPLAY_MODE="none"; shift ;;
        --timeout) BOOT_TIMEOUT="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage; exit 2 ;;
    esac
done

command -v qemu-system-x86_64 >/dev/null 2>&1 || { echo "qemu-system-x86_64 not found (pacman -S qemu-full)" >&2; exit 127; }

# OVMF firmware: paths differ between distributions.
OVMF_CODE="${TAGOS_OVMF_CODE:-}"
OVMF_VARS_SRC="${TAGOS_OVMF_VARS:-}"
if [[ -z "$OVMF_CODE" ]]; then
    for candidate in \
        /usr/share/edk2/x64/OVMF_CODE.4m.fd \
        /usr/share/edk2/x64/OVMF_CODE.fd \
        /usr/share/OVMF/OVMF_CODE.fd \
        /usr/share/ovmf/x64/OVMF_CODE.fd; do
        [[ -f "$candidate" ]] && OVMF_CODE="$candidate" && break
    done
fi
[[ -n "$OVMF_CODE" && -f "$OVMF_CODE" ]] || { echo "OVMF firmware not found (pacman -S edk2-ovmf)" >&2; exit 127; }

if [[ -z "$OVMF_VARS_SRC" ]]; then
    for candidate in \
        /usr/share/edk2/x64/OVMF_VARS.4m.fd \
        /usr/share/edk2/x64/OVMF_VARS.fd \
        /usr/share/OVMF/OVMF_VARS.fd \
        /usr/share/ovmf/x64/OVMF_VARS.fd; do
        [[ -f "$candidate" ]] && OVMF_VARS_SRC="$candidate" && break
    done
fi
[[ -n "$OVMF_VARS_SRC" && -f "$OVMF_VARS_SRC" ]] || { echo "OVMF VARS template not found" >&2; exit 127; }

mkdir -p "$(dirname "$DISK")"
OVMF_VARS="${OVMF_VARS:-$(dirname "$DISK")/OVMF_VARS.fd}"
if [[ ! -f "$OVMF_VARS" ]]; then
    cp "$OVMF_VARS_SRC" "$OVMF_VARS"
fi

if [[ "$CREATE_DISK" -eq 1 && ! -f "$DISK" ]]; then
    echo "== creating $DISK_SIZE disk at $DISK"
    qemu-img create -f qcow2 "$DISK" "$DISK_SIZE" >/dev/null
fi

if [[ -z "$ISO" && ! -f "$DISK" ]]; then
    echo "no disk at $DISK (use --create-disk) and no --iso given" >&2
    exit 1
fi

# virtio-vga-gl (virgl) needs an OpenGL-capable host display; without one QEMU
# exits immediately. Headless boot tests and CI containers get the plain device,
# and `--gpu gl` opts into acceleration when a real GPU is available.
GPU_ARGS=(-device virtio-vga)
if [[ "$GPU_MODE" == "gl" ]]; then
    GPU_ARGS=(-device virtio-vga-gl)
fi

QEMU_ARGS=(
    -machine q35,accel=kvm:tcg
    -cpu max
    -smp "$CPUS"
    -m "$MEMORY"
    -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE"
    -drive if=pflash,format=raw,file="$OVMF_VARS"
    "${GPU_ARGS[@]}"
    -device qemu-xhci
    -device usb-tablet
    -device intel-hda
    -device hda-duplex
    -nic user,model=virtio-net-pci
    -device virtio-blk-pci,drive=root
    -rtc base=localtime
)

if [[ -n "$ISO" && -f "$ISO" ]]; then
    QEMU_ARGS+=(-drive file="$ISO",media=cdrom,readonly=on -boot order=d)
fi

if [[ -f "$DISK" ]]; then
    QEMU_ARGS+=(-drive file="$DISK",if=none,id=root,cache=writeback)
else
    QEMU_ARGS+=(-drive file=/dev/null,if=none,id=root,format=raw)
fi

if [[ "$BOOT_TEST" -eq 1 ]]; then
    SERIAL_LOG="${SERIAL_LOG:-$ROOT/build/vm/boot-test.log}"
    mkdir -p "$(dirname "$SERIAL_LOG")"
    : >"$SERIAL_LOG"
    echo "== boot test: serial log $SERIAL_LOG (timeout ${BOOT_TIMEOUT}s)"
    set +e
    timeout --foreground "$BOOT_TIMEOUT" \
        qemu-system-x86_64 "${QEMU_ARGS[@]}" \
            -display none -serial "file:$SERIAL_LOG" -monitor none
    set -e
    if grep -q "TAGOS_BOOT_OK" "$SERIAL_LOG"; then
        echo "== boot test PASSED"
        exit 0
    fi
    echo "== boot test FAILED (no TAGOS_BOOT_OK marker)" >&2
    tail -n 40 "$SERIAL_LOG" >&2 || true
    exit 1
fi

case "$DISPLAY_MODE" in
    spice)
        QEMU_ARGS+=(-spice port=5930,disable-ticketing=on -device virtio-serial-pci
                    -chardev spicevmc,id=vdagent,name=vdagent
                    -device virtserialport,chardev=vdagent,name=com.redhat.spice.0)
        echo "== SPICE: spicec -h 127.0.0.1 -p 5930   (or: remote-viewer spice://127.0.0.1:5930)"
        ;;
    gtk) QEMU_ARGS+=(-display gtk,gl=on,zoom-to-fit=on) ;;
    none) QEMU_ARGS+=(-display none -serial mon:stdio) ;;
    *) echo "unknown display mode: $DISPLAY_MODE" >&2; exit 2 ;;
esac

if [[ -n "$SERIAL_LOG" ]]; then
    QEMU_ARGS+=(-serial "file:$SERIAL_LOG")
fi

echo "== TagOS VM: ${MEMORY} RAM, ${CPUS} vCPU, UEFI, virtio"
exec qemu-system-x86_64 "${QEMU_ARGS[@]}"

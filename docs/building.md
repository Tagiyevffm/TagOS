# Building TagOS

## Supported build host: Arch Linux

```bash
sudo pacman -S --needed git cmake ninja gcc pkgconf \
    qt6-base qt6-declarative qt6-svg qt6-tools qt6-wayland \
    python shellcheck \
    qemu-full edk2-ovmf \
    archiso squashfs-tools libisoburn
```

### Build and test

```bash
tools/build.sh                       # configure + build + ctest  → build/dev
tools/build.sh --type Debug          # debug build
tools/build.sh --clean --no-tests    # clean rebuild, skip tests
tools/lint.sh                        # qmllint + shellcheck + repo checks
```

### Run the shell on your current session

```bash
QT_QPA_PLATFORM=wayland TAGOS_DRY_RUN=1 ./build/dev/bin/tag-shell
```

`TAGOS_DRY_RUN=1` makes `AppLauncher` and `PowerManager` record actions instead
of spawning processes — handy when you do not want a reboot button to work on
your workstation.

### Development VM (QEMU, UEFI)

```bash
tools/run-qemu.sh --create-disk          # 32 GiB qcow2 in build/vm
tools/run-qemu.sh --iso out/tagos.iso    # boot the live medium over SPICE
```

Defaults match the Phase 0 profile: 3 GiB RAM, 4 vCPU, virtio-gpu, virtio-blk,
virtio-net, Intel HDA, UEFI (OVMF), 1280×800. Override with `--memory`,
`--cpus`, `--disk`, `--display gtk|spice|none`, or the `TAGOS_VM_*` variables.
`TAGOS_OVMF_CODE` / `TAGOS_OVMF_VARS` select non-standard firmware paths.

Serial console (also used by the boot test):

```bash
tools/run-qemu.sh --iso out/tagos.iso --boot-test --serial-log build/vm/boot.log
```

### ISO

```bash
sudo tools/build-iso.sh                      # build + tests + ISO + boot verification
sudo tools/build-iso.sh --skip-build         # reuse build/arch
sudo tools/build-iso.sh --no-boot-test       # skip QEMU
```

The script stages the archiso profile in `build/iso-profile/tagos`, installs the
built binaries into `airootfs` (`DESTDIR` install), generates
`packages.x86_64` from `system/packages/*.txt`, enables the live units, and runs
`mkarchiso`. Output lands in `out/` with a `.sha256` next to it.

## Windows developer host (UI work only)

Requirements: MSYS2 with `ucrt64` toolchain, Qt 6 (base, declarative, svg,
tools), CMake, Ninja.

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base \
    mingw-w64-ucrt-x86_64-qt6-declarative mingw-w64-ucrt-x86_64-qt6-tools \
    mingw-w64-ucrt-x86_64-qt6-svg

cmake -S . -B build/win -G Ninja
cmake --build build/win
ctest --test-dir build/win --output-on-failure
./build/win/bin/tag-shell --smoke-test --log-file build/shell.log
```

What works on Windows: the design system, the whole shell, TagMonitor, the
`tag-system` parsers and samplers, the wire-format codecs, and the QML smoke
test (offscreen). What does not: QtDBus (`tag-flowd`), systemd units, labwc,
archiso, QEMU boot verification.

## Continuous integration

`.github/workflows/ci.yml` runs:

* **build** — Arch container: configure, build, `ctest`, `tools/lint.sh`;
* **added-value checks** — repository checks and QML smoke run in the same job;
* **iso** (manual/dispatch) — build the ISO and boot it headless with
  `tests/integration/boot-test.sh`, uploading the serial log on failure.

## Troubleshooting

| Symptom | Cause / fix |
| --- | --- |
| `Module "TagOS.Shell" contains no type named "Main"` | QML modules are not next to the executable; ensure `QT_QML_OUTPUT_DIRECTORY=<build>/bin` (set by the top-level CMakeLists) and that the app adds `applicationDirPath()` to the import path |
| QML warnings about `Theme.color` being undefined | `Theme.qml` is not registered as a singleton; check `QT_QML_SINGLETON_TYPE` in `desktop/tag-design/CMakeLists.txt` |
| `desktop-shell-smoke` fails with `qml-errors=N` | run the shell with `--log-file` and open the log; the smoke test counts QML runtime errors |
| `tag-flowd` cannot own `org.tagos.Flow` | another instance is running (`systemctl --user status tag-flow`), or the session bus is missing |
| QEMU boot test times out | check `build/vm/boot-test.log` for `TAGOS_BOOT_WAITING`; if the marker is missing the shell never wrote `%t/tagos/ready` |

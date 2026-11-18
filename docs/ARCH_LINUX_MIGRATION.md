# Arch Linux migration — Phase A audit

Status: **Phase A complete** (audit + Linux foundation fixes). Phases B–F are
planned here and tracked in `docs/roadmap.md`.

TagOS is a Linux distribution that owns its desktop environment. Arch Linux is
the foundation (kernel, systemd, pacman, NetworkManager, PipeWire, BlueZ, Mesa,
XDG portals); TagOS owns the session, the shell, the applications and the image.
Nothing in this document replaces an upstream component with a local copy.

---

## 1. How this audit was produced

1. Read `README.md`, `docs/architecture.md`, `docs/phase-0.md`,
   `docs/phase-1.md`, `docs/testing.md`, all five ADRs and every
   `CMakeLists.txt` under `core/`, `apps/`, `desktop/`, `system/`, `tests/`.
2. Grepped the whole tree for host-specific code (`Q_OS_WIN`, `_WIN32`,
   `cmd.exe`, `powershell`, `wmic`, `taskkill`, `gio`, `DETACHED_PROCESS`).
3. Probed the development host for a usable Linux runtime (`docker`, `podman`,
   `wsl`, `qemu-system-x86_64`, `makepkg`, `pacman`, `mkarchiso`, `gcc`,
   `shellcheck`, `gh`) and for git remotes.
4. Installed the current tree into a `DESTDIR` staging root and compared the
   result with what labwc, greetd, the archiso profile and a display manager
   actually read.

## 2. Development environment reality

Measured on the machine this work was done on (Windows 11 + MSYS2 UCRT64):

| Capability | Result |
| --- | --- |
| Native Linux / dual boot | absent |
| WSL 2 (`Ubuntu`) | installed but stopped; every start fails with `Wsl/Service/E_UNEXPECTED` |
| Nested virtualisation (`VirtualizationFirmwareEnabled`, `SLAT`) | `False` — no hypervisor can run |
| `docker`, `podman`, `qemu-system-x86_64` | not installed |
| `makepkg`, `pacman`, `mkarchiso`, `shellcheck`, `gh` | not installed |
| `git remote` | none configured |
| Windows toolchain | MSYS2 UCRT64, GCC 16.2.0, Qt 6.12.0, CMake 4.5.0, Ninja 1.13.2 |

**Decision (mission §8): stop trying to enable virtualisation.** The
authoritative build and test environment is a Linux runner, i.e. the
`archlinux:latest` container jobs in `.github/workflows/ci.yml`. Windows remains
a *development convenience* for QML/UI work and is never evidence that a Linux
feature works. Every result in this document is labelled with where it was
obtained:

* **[win]** — observed on the Windows development host only,
* **[ci]** — produced/observable in the Arch Linux CI job,
* **[linux]** — requires a real Arch + Wayland machine (not yet observed).

## 3. Module audit

Legend for *Action*: **keep** (no change needed), **fixed** (changed in Phase A),
**Phase X** (scheduled), **gap** (known Linux deficiency, documented below).

| Module | What it does | Host assumptions found | Action |
| --- | --- | --- | --- |
| `core/tag-common` | `TAGOS_EXPORT` visibility macro | none (portable) | keep |
| `core/tag-system` | `/proc` samplers, `os-release`, logind power actions, session info, `/sys` thermal, process list, network sampler | `SystemProbeWindows.cpp` compiled only when `WIN32`; Linux probe uses `statvfs`, `/proc`, `/sys/class/thermal`, `QSysInfo` — no Windows API in shared code. `PowerManager` calls `systemctl`/`loginctl`. | keep (see gap G1) |
| `core/tag-config` | one validated JSON settings document in `$XDG_CONFIG_HOME/tagos` | `QStandardPaths::GenericConfigLocation` (XDG-correct); atomic `QSaveFile`; `gsettings` used only to read the *host* colour scheme | keep (see gap G2) |
| `core/tag-control` | network/audio/Bluetooth/power/brightness control | drives `nmcli`, `bluetoothctl`, `wpctl`, `brightnessctl`, `powerprofilesctl`, `upower`, `rfkill`, `gammastep`; availability probed once; missing tool ⇒ permanent disabled tile with a reason | Phase B (see gap G3) |
| `core/tag-fs` | directory model, file operations, search, preview | **was** delegating permanent trash moves to `gio trash` | **fixed**: native FreeDesktop Trash implementation (`tag-fs/Trash.h`), 13 unit tests |
| `core/tag-apps` | Desktop Entry parsing, XDG application catalog, launching | none; parses `Name[lang]`, `Exec` field codes, `TryExec`, `OnlyShowIn`, searches `XDG_DATA_DIRS` | keep |
| `core/tag-runtime` | single GUI bootstrap: QML import paths, `--smoke-test`, `--screenshot`, `--ready-file`, logging | none | keep |
| `core/tagflow` | `tag-flowd` D-Bus service + `FlowClient` fallback | QtDBus optional at build time; falls back to in-process sampling | keep (see gap G4) |
| `core/tag-session` | POSIX `sh` session launcher | already Linux-only: exports `XDG_SESSION_TYPE=wayland`, `QT_QPA_PLATFORM=wayland`, verifies `labwc`, activates the systemd user environment, hands over with `exec` | keep |
| `desktop/tag-design` | theme tokens + 19 QML components + 79 icons ×2 themes | none | keep |
| `desktop/tagshell` | panel, dock, launcher, control centre, notifications | renders inside one XDG toplevel (`Main.qml` window); layer-shell promotion is a wrapper change | Phase C (see gap G5) |
| `apps/tag-files` | file manager | used `FileOps::moveToTrash` → `gio` | **fixed** (native trash) |
| `apps/tag-settings` | 6 settings pages backed by `SystemControl`/`SettingsStore` | none | keep |
| `apps/tag-monitor` | CPU/memory/disk/network/process monitor | none; `/proc`-based | keep |
| `apps/tag-about` | system information | none | keep |
| `system/systemd` | 3 user units + 1 system unit | Linux-only by nature | keep |
| `system/config` | labwc, greetd, `environment.d` | were installed into `/usr/share/tagos/*`, where labwc/greetd never look | **fixed**: `/etc/xdg/labwc`, `/etc/greetd` |
| `installer/archiso` | live profile, UEFI systemd-boot entries, autologin | needs `mkarchiso` | Phase E [linux] |
| `packaging/` | Arch package | **was missing entirely** | **added**: `PKGBUILD`, `tagos.install`, `README.md` |
| `tools/` | build, lint, ISO, QEMU | Windows-only dev script and ISO/QEMU scripts existed; no Arch-native build/package entry point | **added**: `build-arch.sh`, `test-linux.sh`, `package-arch.sh` |

### Confirmed reusable as-is

The whole QML layer (shell + 4 applications + design system, 6 185 lines /
42 files), the C++ module boundaries, `tagflow`'s wire codec, the `/proc`
parsers and the settings store needed no porting: they were written against
Linux APIs from the start. **No module was rewritten in Phase A.**

### Confirmed host-specific (and therefore not evidence of Linux support)

* `SystemProbeWindows.cpp` — the `WIN32`-only probe that lets the metrics CLI
  run on a developer workstation. It feeds `tagos-metrics` only; it is not part
  of the session.
* `FlowClient`'s in-process fallback when QtDBus is absent.
* QML smoke tests and screenshots taken with `QT_QPA_PLATFORM=offscreen`.

## 4. Changes made in Phase A

### 4.1 FreeDesktop Trash (replaces `gio trash`)

`core/tag-fs/include/tag-fs/Trash.h`, `src/Trash.cpp` implement the
specification directly:

* home trash `$XDG_DATA_HOME/Trash/{files,info}`,
* per-filesystem trash `<mount>/.Trash-$uid` (fallback `<mount>/.Trash/$uid`),
  chosen by comparing `st_dev` of the file and of the home trash,
* `.trashinfo` with `Path=` percent-encoded and `DeletionDate=` in local time,
* rename on the same filesystem; on `EXDEV` a copy that **preserves symlinks as
  symlinks** and never follows them,
* collision suffixes (`name.2.txt`), atomic `QSaveFile` info writes with rollback
  of the payload if the info file cannot be written,
* `restore()` never overwrites an existing file (it restores next to it),
* `empty()` is the only permanent deletion and is reachable only after the UI
  confirmation added in Phase 1.

`FileOps::moveToTrash` now uses this service; TagOS no longer depends on `glib2`
for deletion. Tests: `tests/unit/tst_trash.cpp` (13 cases; the symlink case is
POSIX-only and skips on Windows).

### 4.2 System configuration in the right place

`CMakeLists.txt` installed labwc and greetd configuration to
`/usr/share/tagos/{labwc,greetd}`, where neither program looks. They now install
to `/etc/xdg/labwc` and `/etc/greetd` via a cache variable
`TAGOS_SYSCONF_DIR` (default `/etc`). Staged installs use `DESTDIR`, the distro
convention, so a `cmake --install` never writes to the build host.

### 4.3 Session entry and application icons

* `system/wayland-sessions/tagos.desktop` — `Exec=/usr/bin/tag-session`,
  `TryExec`, `DesktopNames=TagOS`. Without this file no display manager can list
  the session.
* The four application icons plus `tagos-logo` are installed into
  `share/icons/hicolor/scalable/apps`, so `Icon=` resolves outside TagShell.

### 4.4 Arch packaging

`packaging/PKGBUILD` (base desktop package), `packaging/tagos.install`,
`packaging/README.md`. The package builds the desktop stack from a `git archive`
tarball, runs `ctest` in `check()`, installs with `DESTDIR`, and declares the
upstream services it integrates with (`labwc`, `networkmanager`, `bluez`,
`pipewire`, `wireplumber`, `upower`, `power-profiles-daemon`, `polkit`,
portals) instead of vendoring them.

### 4.5 Linux tooling

| Script | Purpose |
| --- | --- |
| `tools/build-arch.sh` | Arch configure + build + `ctest`; `--deps` installs build dependencies, `--install DIR` does a `DESTDIR` staging install |
| `tools/test-linux.sh` | refuses to run off Linux; `ctest`, headless app starts, `tagos-metrics`, `/proc` + thermal checks, optional `dbus-run-session` `tag-flowd`, lint |
| `tools/package-arch.sh` | reproducible `git archive` tarball → `makepkg` → `.pkg.tar.zst` + `.sha256` |
| `tools/build-iso.sh` | (existing) now installs with `--prefix /usr` under `DESTDIR` and **fails early** if any of 10 required payload files is missing |

### 4.6 CI

`.github/workflows/ci.yml` has three jobs, all on `archlinux:latest`:

1. **build-and-test** — configure with `/usr` + `/etc`, build, `ctest`,
   `tools/lint.sh`, `tag-flowd` on a real session bus, `tools/test-linux.sh`,
   and a `DESTDIR` install-tree assertion of 17 required paths.
2. **package-arch** — unprivileged `makepkg`, then unpacks the package and
   asserts the session entry, units, icons and `/etc` configuration are inside.
3. **iso-boot** (manual dispatch) — `tools/build-iso.sh` + headless UEFI boot.

### 4.7 Repository checks

`tests/system/test_system_files.py` gained: session-entry validity, PKGBUILD
version parity with the CMake project, mandatory package dependencies, presence
of `tagos.install`, non-stub Linux tool scripts, and `.desktop` → icon coverage.
It now reports `4 units, 82 package entries, 79 icons, 4 desktop entries`.

## 5. Gaps and their schedules

Every gap is a deliberate omission with a reason, not an undiscovered defect.

| # | Gap | Why it is still open | Phase |
| --- | --- | --- | --- |
| G1 | No polkit-based shutdown/suspend via logind D-Bus; `PowerManager` shells out to `loginctl`/`systemctl` | works today, but a D-Bus call gives a real polkit prompt and no CLI dependency | B |
| G2 | Settings document has no schema version / migration path | only one document shape exists so far; migration code without a second shape would be untestable | B |
| G3 | `SystemControl` uses CLI adapters and polling instead of NetworkManager/BlueZ/UPower/PipeWire D-Bus signals | must be replaced by `org.tagos.Network1`/`Audio1`/`Notify1`/`Power1` services so QML receives signals; CLI stays as a fallback | B |
| G4 | `tag-flowd` D-Bus path never executed on a Linux host by this work | needs the CI D-Bus step to be observed green (job exists, no run recorded yet) | B |
| G5 | Panel/dock/control centre are XDG toplevels, not `wlr-layer-shell` surfaces; single monitor only | exclusive-zone behaviour can only be validated on a real Wayland compositor | C |
| G6 | No installer: the live image autologs in but cannot install to disk | requires a Linux host to build and boot the ISO | E |
| G7 | ISO never built or booted during this work | `mkarchiso` + QEMU/UEFI need Linux | E |
| G8 | No TagTerminal | no PTY backend yet; shipping a fake terminal was rejected | 3 |
| G9 | No `LICENSE` file in the tree | the code declares `GPL-3.0-or-later` in two places (PKGBUILD and TagAbout), but the licence decision and text belong to the project owner. Packaging now warns instead of silently skipping, and the repo check keeps the two declarations consistent. **Owner decision requested.** | owner |
| G10 | Trash is per-user and lists only the home trash | cross-filesystem trashes are written correctly but not yet aggregated in the UI | 3 |

## 5b. Validation gate (Phase A, second pass)

The gate ran everything that a non-Linux host can run and found four defects that
a Windows host hides:

| # | Defect | Impact on Linux | Fix |
| --- | --- | --- | --- |
| V1 | every script committed as `100644` (repo initialised with `core.filemode=false`) | `run: tools/lint.sh` in CI and `tag-session` would be "Permission denied" | all ten scripts set to `100755` in the index |
| V2 | nothing checked case-sensitive includes/imports | `#include "tag-system/procfs.h"` compiles on Windows, fails on Arch | new `repo-case-and-paths` CTest |
| V3 | nothing checked CRLF/BOM in scripts, units, `.desktop`, `PKGBUILD` | shebang `bash^M` and BOM'd unit files fail on Linux | same test |
| V4 | `PKGBUILD` skipped the missing `LICENSE` with `|| true` | the package would claim `GPL-3.0-or-later` and ship no licence text, silently | explicit warning + repo check that the two licence declarations agree |

Executed on the gate host (Windows, so: structural validation only):

* 15/15 CTest suites green, including the new case/path test over 375 files.
* `tools/lint.sh` green including `shellcheck` 0 findings; `actionlint 1.7.12`
  reports 0 problems for `.github/workflows/ci.yml` (schema + embedded shell).
* All 25 `depends` and 5 `makedepends` of the PKGBUILD resolved to real packages
  in `core`/`extra` through the archlinux.org package API.
* The `git archive` tarball makepkg consumes: 480 entries, 989 KiB, correct
  755/644 modes, no `build/` or `out/` leakage.
* `package()` executed for real against a configured build tree with makepkg
  semantics (`cd $srcdir`, `set -e`): exit 0, 219 files, `/etc/xdg/labwc`,
  `/etc/greetd`, `wayland-sessions/tagos.desktop`, hicolor icons, both systemd
  unit sets and `usr/share/tagos/tagos.install` all present.

Still **not** executed anywhere: `makepkg` metadata generation and dependency
installation, the `ctest` run on Arch (GCC 16 / Qt 6.12 there instead of MSVC-less
MinGW), the CI jobs, `mkarchiso`, QEMU/UEFI boot and the installer. Those need a
Linux host; the gate host has none (see §2), and the repository has no git remote,
so GitHub Actions cannot be triggered from here.

## 6. What Phase A does and does not prove

**Proven [win]:** the tree compiles with GCC 16 / Qt 6.12, all 14 CTest suites
pass headless (including the new Trash suite), the design system and shell still
render, `DESTDIR` install produces the expected layout, repository checks pass.

**Proven structurally [ci]:** the CI definition builds, tests, lints, packages
and asserts the install tree on Arch Linux. The jobs are committed but have not
run in this environment (no git remote, no runner access).

**Not proven anywhere yet [linux]:** Wayland session start, layer-shell
behaviour, real `nmcli`/`wpctl`/`bluetoothctl`/`upower` round-trips, logind power
actions, `tag-flowd` on the session bus in a real session, ISO boot, installer,
disk installation. Claiming any of these without a run would be dishonest.

## 7. Phase B–F plan (concrete, in order)

**B — native system integration**
1. `core/tag-control`: replace `nmcli` polling with `org.freedesktop.NetworkManager`
   D-Bus (`AccessPoint`, `Device.Wireless`, `Settings`) behind the same QML
   property names; keep the CLI adapter as fallback.
2. `tag-audio`: PipeWire via `org.freedesktop.mpe.Settings`/`wpctl` first, then a
   `WirePlumber` client when the API is stable.
3. `tag-power`: logind `org.freedesktop.login1.Manager` + UPower +
   power-profiles-daemon D-Bus, with polkit prompts instead of CLI.
4. `SettingsStore`: add `schemaVersion` + migration step, tested with a fixture
   document from the previous schema.
5. `tag-flowd`: subscribe to udev/netlink events instead of a 1 s timer.

**C — Wayland desktop**
6. Promote `TopPanelContent`/`DockContent`/`ControlCenterContent` into
   `wlr-layer-shell` surfaces (`layer-shell-qt`), with exclusive zones.
7. Multi-monitor: per-output panel instances, `QScreen` change handling.
8. XDG activation (`xdg_activation_v1`) for launching focused windows.

**D — packaging**
9. Split `tagos` (runtime) and `tagos-devel`; add a `tagos-meta` that pulls the
   session dependencies.
10. Sign the package, add a `pacman.conf` repo entry in the ISO.

**E — ISO and installer**
11. Real installer (`tag-installer`, QML + `archinstall`-free): partition
    preview, explicit destructive confirmation, `pacstrap`, `genfstab`,
    `mkinitcpio`, `systemd-boot`/GRUB selection, user creation, session enable.
12. Verify BIOS and UEFI boot, secure-boot-off, and the installed system's first
    boot in QEMU.

**F — validation**
13. Idle RAM/startup measurements on the QEMU profile (3 GiB / 4 vCPU) and on
    metal; publish real numbers with the measurement method.
14. Soak test: shell under 8 h with `tag-flowd` and one app crashing, proving the
    D-Bus boundary keeps the desktop alive.

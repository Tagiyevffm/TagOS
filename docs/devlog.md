# TagOS development log

Chronological record of the work done on the Arch Linux migration and validation.
Every entry says what was changed, why, and where the evidence came from
(Windows development host / Arch Linux CI / QEMU).

## Phase 0 — Foundation (v0.1.0)

Arch base, labwc, Qt 6/QML shell skeleton, TagFlow metrics service, systemd user
units, archiso profile, QEMU scripts. Windows was the only available build host;
Linux behaviour was never executed.

## Phase 1 — Desktop (v0.2.0 "Aurora")

Themed shell (panel, dock, launcher, control centre), settings store
(`TagOS.Config`), application catalog (`TagOS.Apps`), system adapters
(`TagOS.Control`), filesystem module (`TagOS.Files`), TagFiles/TagSettings/
TagMonitor/TagAbout. 13 CTest suites. Still Windows-only verification.

## Phase A — Arch Linux audit and migration

* Audited every module; found the tree already Linux-first, with a single
  Windows-only probe (`SystemProbeWindows.cpp`, `WIN32`-guarded).
* Replaced `gio trash` with a native FreeDesktop Trash implementation
  (`TagOS::Files::TrashService`) — `st_dev` based per-filesystem trash,
  percent-encoded `.trashinfo`, symlink-preserving cross-device moves,
  non-overwriting restore.
* Fixed the install layout: labwc/greetd configuration moved from
  `/usr/share/tagos` (where neither program looks) to `/etc/xdg/labwc` and
  `/etc/greetd`; added `/usr/share/wayland-sessions/tagos.desktop` and hicolor
  application icons.
* Added `packaging/PKGBUILD`, `packaging/tagos.install`, `packaging/README.md`,
  `tools/build-arch.sh`, `tools/test-linux.sh`, `tools/package-arch.sh`.
* CI moved to three `archlinux:latest` jobs.

## Phase A validation (GitHub Actions, real Arch Linux)

The repository was connected to `github.com/Tagiyevffm/TagOS` and the gate ran
against real Arch Linux. Each fix below is a defect that the Windows host could
not reveal:

| # | Symptom in CI | Real cause | Fix |
| --- | --- | --- | --- |
| 1 | `Permission denied` for scripts | all scripts committed `100644` (repo initialised with `core.filemode=false`) | `git update-index --chmod=+x` on all ten; CTest asserts the mode |
| 2 | `relocation R_X86_64_PC32 ... recompile with -fPIC` | static `tagos-system` linked into the shared QML module | `CMAKE_POSITION_INDEPENDENT_CODE ON` |
| 3 | package job died at step 1 | wrote `/etc/sudoers.d/builder` in a container without sudo | removed; not needed |
| 4 | `qmllint not found` (exit 127) | Arch installs it at `/usr/lib/qt6/bin/qmllint`, not on `PATH` | resolved via `qtpaths6` + known layouts |
| 5 | `Cannot find the debugedit binary` | Arch enables debug packages by default | `options=('!strip' '!debug')` (binaries ship unstripped on purpose) |
| 6 | `check()` failed: "not tracked by git" | the test ran `git ls-files` inside a tarball, which walked up to the parent checkout | use the index only when the tree owns `.git`; otherwise the tar mode |
| 7 | `file COPY cannot make directory "/usr/lib/qml/TagOS"` | `install(CODE)` ignored `$ENV{DESTDIR}` — **no QML modules were installed at all** | `install(DIRECTORY ... DESTINATION lib/qml)` |
| 8 | path checker: `'Runtime' does not exist` | `COMPONENT Runtime` parsed as a source path | keywords stripped with their argument |
| 9 | `install: cannot stat .../src/tagos.install` | the install script was never copied into `$srcdir` | listed in `source=()` |
| 10 | makepkg: `target not found: fonts`, `systemd-sysusers`, `systemd-tmpfiles` | package lists contained binaries and a non-existent package | removed; CI now validates every name with `pacman -Si` |
| 11 | payload check: shell syntax error | broken line continuation in the workflow | rewritten as a proper list |

Evidence: `build-and-test` green (build, 15/15 CTest, lint, D-Bus snapshot,
`tools/test-linux.sh`, 17-path install assertion); `tagos 0.2.0-1-x86_64.pkg.tar.zst`
produced by `makepkg` in CI.

## Features added for the distribution

* **Firefox**: added to `system/packages/desktop.txt`, default handler for
  `http`/`https`/`text/html` in `/etc/xdg/mimeapps.list`, and pinned in the dock.
  Firefox is a real Arch package and runs natively on Wayland; TagOS ships no
  browser engine.
* **Design system enforcement**: five QML files spelled hex colours directly;
  they are now `Theme` tokens and `repo-system-units` fails on any colour literal
  outside `Theme.qml`.
* **VM without host OpenGL**: `run-qemu.sh` demanded `virtio-vga-gl`, which exits
  immediately in a container or headless TCG run. Plain `virtio-vga` is now the
  default, `--gpu gl` opts in.
* **Docs**: `docs/install.md` (live medium, package install, session flow,
  uninstall), `docs/ARCH_LINUX_MIGRATION.md` (audit + gap list).

## Open items

* `LICENSE`: the code declares `GPL-3.0-or-later` in two places, the owner has
  not confirmed it, so no licence text is shipped and packaging warns.
* Layer-shell panels (Phase C), D-Bus network/audio/power services (Phase B),
  installer wizard (Phase E), TagTerminal (no PTY backend yet).
* The ISO boot test result is recorded in `TAGOS_FINAL_REPORT.md`.

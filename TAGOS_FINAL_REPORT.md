# TagOS — final report

Written at the end of the automated work session on `main`
(`https://github.com/Tagiyevffm/TagOS`). Every claim below is labelled with where
it was observed: **Arch CI** (GitHub Actions, `archlinux:latest`), **QEMU**
(headless UEFI boot inside that runner) or **Windows** (development host, not
evidence of Linux behaviour).

## 1. Is TagOS actually bootable?

**Yes — observed, not inferred.**

Run [`44420960594`](https://github.com/Tagiyevffm/TagOS/actions/runs/44420960594)
built the ISO with `mkarchiso`, booted it under UEFI (OVMF) in QEMU with TCG
acceleration, and read the serial console. The guest printed:

```
TAGOS_BOOT_WAITING timeout=600s
TAGOS_BOOT_OK session={"component":"tag-shell","host":"archlinux",
                       "pid":670,"platform":"wayland","version":"0.2.0"}
TAGOS_CHECK tag-shell=running
TAGOS_CHECK tag-files=launched
TAGOS_CHECK firefox=launched
TAGOS_CHECK firefox-package=installed Mozilla Firefox 160.0.1
TAGOS_SELFTEST_DONE
```

The session was up **29 seconds** after the boot marker started waiting, and
`platform":"wayland"` is read from the shell itself, so this proves a Wayland
compositor, a D-Bus user session and a rendering Qt shell — not just a kernel.

Artifact: `out/tagos-2026.11.18-x86_64.iso.sha256` plus
`build/vm/boot-test.log` in the `tagos-iso` artifact of that run.

Getting there took eleven ISO builds; each one exposed a distinct real defect and
all of them are fixed on `main` (see `docs/devlog.md`). The last four were:

| Symptom on the serial console | Real cause |
| --- | --- |
| text login, no session | greetd 1.10 exits 1 in 14 ms with `no default_session specified`; the live config only had `[initial_session]` |
| session opens and closes in under a second | `mkarchiso` copies the overlay with `cp -a --no-preserve=mode`, so every executable CMake installed arrived as `0644` and could not be executed |
| compositor starts, no shell | `XDG_SESSION_TYPE` was inherited as `tty` from PAM, and wlroots could not get EGL from virtio-gpu under TCG |
| shell running 11 min, marker never fires | `ProtectSystem=strict` mounts `/run` read-only, so `--ready-file %t/tagos/ready` failed until the unit declared `RuntimeDirectory=tagos` |

## 2. Does it start a graphical desktop?

**Yes.** The chain is now observed end to end on the booted ISO:

```
greetd (live: initial_session autologin as tagos)
  └── /usr/bin/tag-session        XDG_SESSION_TYPE=wayland, live media uses pixman
        └── dbus-run-session -- labwc        compositor (WAYLAND_DISPLAY=wayland-0)
              └── /etc/xdg/labwc/autostart   imports env, starts the user target
                    ├── tag-flow.service     D-Bus metrics (org.tagos.Flow1)
                    └── tag-shell.service    --ready-file %t/tagos/ready
```

`tag-shell` was `active (running)` with 14 threads and its ready marker written;
`tag-files` and `firefox` both launched against the live Wayland socket, which is
why `TAGOS_CHECK` reports `launched` and not merely `found`.

Known defect on this path, not yet fixed: `tag-flow.service` restart-loops because
the user manager never received `DBUS_SESSION_BUS_ADDRESS`, and `tag-flowd` exits
with status 2 instead of waiting for the bus. The autostart now imports the
variable (commit after this run); the shell itself degrades to in-process sampling,
which is why the desktop is usable regardless.

## 3. Does Firefox work?

**Tested on the booted ISO.** Firefox 160.0.1 is a real Arch package in
`system/packages/desktop.txt`, it is the default handler for `http`/`https`/
`text/html` via `/etc/xdg/mimeapps.list`, it is pinned in the dock, and
`tag-session` exports `MOZ_ENABLE_WAYLAND=1`, so it starts as a Wayland client.

On the booted image the marker reported
`TAGOS_CHECK firefox=launched` (a `firefox` process alive on
`WAYLAND_DISPLAY=wayland-0`) and
`TAGOS_CHECK firefox-package=installed Mozilla Firefox 160.0.1`. What is not
verified is page rendering inside the VM: the boot test is headless, so nobody has
looked at a rendered page.

## 4. Can a user install it?

**No installer.** There is no wizard, no partitioning step and no first-boot
setup. The live image carries `archinstall`, `parted` and `gptfdisk`, so an
experienced user can install Arch manually and then install the `tagos` package,
which *is* proven to build and to have a correct payload. That is a documented
manual path, not an installer.

## 5. What was tested on real Linux

Everything in this section ran on **Arch Linux** in GitHub Actions
(`archlinux:latest`, GCC 16.2, Qt 6.12, CMake 4.5, Ninja 1.13):

| Check | Result |
| --- | --- |
| CMake configure + full build (401 targets) | pass |
| `ctest` — 15 suites (unit, models, parsers, trash, QML smoke, repo checks) | **15/15 pass** |
| `tools/lint.sh` (qmllint + shellcheck + repo checks) | pass |
| `tag-flowd --once --json` on a real session bus | pass (schema 1, live memory data) |
| `tools/test-linux.sh` (headless app starts, `/proc`, thermal, lint) | pass |
| Install-tree assertion (19 paths incl. QML modules, session entry, units) | pass |
| Package dependency resolution (25 runtime deps from `core`/`extra`) | pass |
| `makepkg` build + `check()` (full ctest inside the package build) | pass → `tagos 0.2.0-1-x86_64.pkg.tar.zst` |
| Package payload inspection (binaries, QML, units, session, icons, `/etc`) | pass |
| `mkarchiso` ISO build | pass (ISO produced) |
| ISO UEFI boot in QEMU (OVMF + TCG) | **pass** — `TAGOS_BOOT_OK`, Wayland session, shell, Files, Firefox |
| Post-boot self-test on the live desktop | pass — `TAGOS_CHECK` x4, `TAGOS_SELFTEST_DONE` |

Also verified on the **Windows** development host only (not proof of Linux
behaviour): the 15 test suites, the QML smoke tests and a staged install from
which `tag-shell` runs.

## 6. Which features remain incomplete

* **`tag-flow.service` on the live image** — restart-loops; `DBUS_SESSION_BUS_ADDRESS`
  is not in the user manager environment and `tag-flowd` exits instead of retrying.
* **Live renderer** — the live medium forces `WLR_RENDERER=pixman` because QEMU TCG
  has no working GL for virtio-gpu. Installed systems keep the GPU path.
* **Installer wizard** — not started (Phase E).
* **Layer-shell panels** — panel/dock/control centre are one XDG window; the
  `*Content.qml` split makes promotion a wrapper change (Phase C).
* **D-Bus system services** — network/audio/power still drive CLI adapters
  (`nmcli`, `wpctl`, `bluetoothctl`, `upower`) with polling instead of
  NetworkManager/UPower/PipeWire signals (Phase B).
* **TagTerminal** — removed; no PTY backend yet.
* **Licence** — code declares `GPL-3.0-or-later` in `packaging/PKGBUILD` and in
  TagAbout, but no `LICENSE` file is shipped and the owner has not confirmed the
  licence. Packaging warns instead of pretending.
* **Multi-monitor, workspaces, notifications persistence** — not implemented.

## 7. What must happen next

1. Fix `tag-flow.service` on the live image (bus address in the user manager) and
   re-run the boot test so the whole desktop stack is green, not just the shell.
2. Publish the ISO produced by run `44420960594` as a release artifact.
3. Then the installer (Phase E), then layer-shell and the D-Bus services
   (Phases C/B in `docs/roadmap.md`).
4. Decide the licence.

## Deliverables in the repository

| Deliverable | Location | State |
| --- | --- | --- |
| Source | `core/`, `apps/`, `desktop/`, `system/`, `installer/` | builds on Arch |
| archiso profile | `installer/archiso/tagos/` | builds an ISO; boot fix committed |
| Package | `packaging/PKGBUILD`, `packaging/tagos.install` | builds in CI |
| Installation instructions | `docs/install.md` | current |
| Browser integration | `system/packages/desktop.txt`, `system/config/mimeapps.list`, dock pins | integrated, untested at runtime |
| Build scripts | `tools/build-arch.sh`, `tools/build-iso.sh`, `tools/package-arch.sh`, `tools/test-linux.sh`, `tools/run-qemu.sh` | used by CI |
| Automated tests | `tests/unit/*`, `tests/system/*`, `tests/integration/boot-test.sh` | 15 CTest suites green on Arch |
| CI | `.github/workflows/ci.yml` (build-and-test, package-arch, iso-boot) | build + package green |
| Development log | `docs/devlog.md` | complete |
| Migration audit | `docs/ARCH_LINUX_MIGRATION.md` | complete |
| ISO artifact | `out/tagos-2026.11.18-x86_64.iso` inside the CI job | built, not bootable yet |

### Commit trail (this session)

```
f938026  Give the live initramfs the CD-ROM/SCSI stack
f844e44  Install the archiso initcpio hook package and drop dracut
17fc789  Keep archiso in one package list
9f052c1  Ship the archiso package in the live image
3b94aa2  Restore the archiso hook in the live initramfs
aa2f2e2  Wait for the user systemd manager in labwc's autostart
c6281e7  Fix the ISO package list and the CI payload check
7e76266  Make the VM usable without host OpenGL and raise the CI boot budget
db62780  Enforce the design system: no colour literals outside Theme.qml
3dd2848  Integrate Firefox as the system browser and fix the PKGBUILD path
52a9543  Stop the path checker from treating install() keywords as paths
9bca52c  Fix QML modules not being installed at all
2e91d1e  Make the executable-bit check correct for a packaging tarball
fe126a7  Fix the lint path resolution and the tarball case of the path test
516b331  Fix the lint and makepkg failures from the second Arch run
39150e3  Fix the two failures the first Arch CI run found
e7b81b2  Record the Phase A validation gate results and the licence question
```

### CI

* Repository: <https://github.com/Tagiyevffm/TagOS>
* Green run (build + tests + lint + package): run `44308672855`, and every run
  after `c6281e7`.
* ISO runs: `44307589268`, `44308672453`, `44312698574`, `44317290407`,
  `44322708001` — every one **built an ISO** and every one failed the boot
  marker; each failure produced one of the commits listed above and moved the
  failure one stage further (package list → initramfs hook → medium detection).

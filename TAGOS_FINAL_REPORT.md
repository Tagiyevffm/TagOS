# TagOS — final report

Written at the end of the automated work session on `main`
(`https://github.com/Tagiyevffm/TagOS`). Every claim below is labelled with where
it was observed: **Arch CI** (GitHub Actions, `archlinux:latest`), **QEMU**
(headless UEFI boot inside that runner) or **Windows** (development host, not
evidence of Linux behaviour).

## 1. Is TagOS actually bootable?

**Partially — and this is the honest answer.**

* The ISO **builds** in CI: `out/tagos-2026.11.18-x86_64.iso`, produced by
  `mkarchiso` from the profile in `installer/archiso/tagos` with 71 packages.
* The ISO **did not boot** in the automated UEFI/QEMU test in the runs completed
  during this session. The initramfs could not mount the live root:
  `Failed to mount '' on real root`, because the `archiso` mkinitcpio hook was
  missing from the image. The hook ships in `mkinitcpio-archiso`, which the live
  package list did not include.
* That defect is **fixed on `main`** (commit `f844e44`) and a fresh ISO build
  with the fix was dispatched. Its result is *not* in this report; treat the boot
  as **unverified** until that run is green.

So: the OS is not yet proven bootable. The build path is proven, the boot path is
one committed fix away from being testable.

## 2. Does it start a graphical desktop?

**Not verified.** The session chain is implemented and reviewed, but it has never
been observed on a running system:

```
greetd (live: initial_session autologin)
  └── /usr/bin/tag-session        exports XDG_SESSION_TYPE=wayland, QT_QPA_PLATFORM=wayland
        └── labwc                 compositor, reads /etc/xdg/labwc/{rc.xml,autostart}
              └── systemctl --user start tag-session.target
                    ├── tag-flow.service    (D-Bus metrics, org.tagos.Flow1)
                    └── tag-shell.service   (--ready-file %t/tagos/ready)
```

What *is* verified (Arch CI): `tag-shell` starts headless and instantiates the
whole QML tree (`desktop-shell-smoke`), and the same binary runs from an installed
tree using only `<exe>/../lib/qml` as its import path (`TAGOS_SMOKE_OK`).

## 3. Does Firefox work?

**Integrated, not tested.** Firefox is a real Arch package added to
`system/packages/desktop.txt`, it is the default handler for
`http`/`https`/`text/html` via `/etc/xdg/mimeapps.list`, it is pinned in the dock,
and `tag-session` exports `MOZ_ENABLE_WAYLAND=1`. Launching it was never possible
in this session: no Linux host, and the live session has not booted yet.

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
| ISO UEFI boot in QEMU | **fail** — initramfs could not mount live root; fix committed, re-run pending |

Also verified on the **Windows** development host only (not proof of Linux
behaviour): the 15 test suites, the QML smoke tests and a staged install from
which `tag-shell` runs.

## 6. Which features remain incomplete

* **ISO boot** — fix committed (`mkinitcpio-archiso`), re-run pending.
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

1. Watch the ISO build dispatched with `f844e44`; if `TAGOS_BOOT_OK` appears on
   the serial console, the boot milestone is proven and the artifact can be
   published.
2. If boot still fails, read `build/vm/boot-test.log` from that run: the marker
   service, greetd autologin and the labwc autostart are the remaining links.
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
* ISO runs: `44307589268`, `44308672453`, `44312698574`, `44317290407` — all
  built an ISO and all failed the boot marker; each failure produced one of the
  commits listed above.

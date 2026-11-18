# Installing TagOS

TagOS is an Arch Linux system with its own desktop environment on top. There are
two supported ways to run it.

## 1. Live medium (recommended for testing)

```bash
# On an Arch Linux machine (root required: mkarchiso uses loop devices and chroots)
sudo tools/build-iso.sh

# The ISO lands in out/ together with its SHA-256:
ls -lh out/tagos-*.iso out/*.iso.sha256

# Boot it headless to check that the session comes up
tools/run-qemu.sh --iso out/tagos-*.iso --boot-test --serial-log build/vm/boot.log
# or interactively, with a SPICE display
tools/run-qemu.sh --iso out/tagos-*.iso
```

The live image boots with systemd-boot (UEFI), auto-logs in as `tagos` through
greetd/tuigreet and starts the TagOS session. It carries `archinstall`, `parted`
and `gptfdisk` if you want to install manually from the live environment.

QEMU defaults: 3 GiB RAM, 4 vCPU, virtio-gpu/blk/net, Intel HDA, OVMF, 1280×800.

## 2. Existing Arch installation (package)

```bash
# Build the package on Arch (as your user, not root)
tools/package-arch.sh                # → out/pkg/tagos-<ver>-<rel>-x86_64.pkg.tar.zst

# Install it
sudo pacman -U out/pkg/tagos-*.pkg.tar.zst
```

The package installs the shell, applications, QML modules, systemd user units,
session entry, icons and `/etc` configuration. It does **not** enable anything
system-wide by itself.

### Start the session

With a display manager that lists Wayland sessions (greetd/tuigreet, GDM, SDDM):
pick **TagOS**.

From a TTY:

```bash
tag-session
```

Verify the session is registered:

```bash
cat /usr/share/wayland-sessions/tagos.desktop
```

### What the session runs

```
greetd / TTY
  └── tag-session            (POSIX sh: exports XDG_*, XDG_SESSION_TYPE=wayland)
        └── labwc            (compositor; owns window management and keys)
              └── /etc/xdg/labwc/autostart
                    └── systemctl --user start tag-session.target
                          ├── tag-flow.service   (D-Bus metrics service)
                          └── tag-shell.service  (panel, dock, launcher, apps)
```

Everything runs as the logged-in user. No TagOS process runs as root; power
actions go through logind/polkit when the user asks for them.

### Browser, files, settings

* Firefox is installed as part of `system/packages/desktop.txt` and set as the
  default handler for `http`/`https`/`text/html` in `/etc/xdg/mimeapps.list`.
* TagFiles, TagSettings, TagMonitor and TagAbout ship as their own binaries with
  `.desktop` entries in `/usr/share/applications`.
* TagLaunch (Ctrl+Space) lists everything installed on the system, not a curated
  fake catalog.

## Uninstall

```bash
sudo pacman -Rns tagos
```

User data in `~/.config/tagos` and `~/.local/share/Trash` is left untouched; the
package `pre_remove` message says so before removal.

## Verifying a build

```bash
tools/build-arch.sh --deps      # configure, build, ctest on Arch Linux
tools/test-linux.sh             # ctest + headless app starts + /proc + lint
sudo tools/build-iso.sh         # ISO + boot verification
```

CI does the same on `archlinux:latest` (see `.github/workflows/ci.yml`); the
`package-arch` job also unpacks the built package and asserts its payload.

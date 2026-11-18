# Arch packaging

TagOS is packaged as a normal Arch package. It installs the desktop stack
(shell, applications, session, units, configuration) **on top of** an ordinary
Arch Linux system; it never replaces the kernel, systemd, NetworkManager,
PipeWire or BlueZ.

## Build the package

```bash
tools/package-arch.sh                 # out/pkg/tagos-<ver>-<rel>-x86_64.pkg.tar.zst
tools/package-arch.sh --install-deps  # install makedepends first (sudo)
```

The script creates `tagos-<ver>.tar.gz` with `git archive HEAD`, so the package
is built from a commit and never from uncommitted working-tree state (it warns if
the tree is dirty). `makepkg` refuses to run as root, so the script must be run
as your user.

Run without the wrapper:

```bash
git archive --format=tar.gz --prefix=tagos-0.2.0/ -o tagos-0.2.0.tar.gz HEAD
cp packaging/PKGBUILD packaging/tagos.install .
makepkg -f
```

## What the package installs

| Path | Contents |
| --- | --- |
| `/usr/bin/tag-shell`, `tag-files`, `tag-settings`, `tag-monitor`, `tag-about` | applications |
| `/usr/bin/tag-session` | session launcher (POSIX `sh`) that `exec`s labwc |
| `/usr/bin/tag-flowd`, `tagos-metrics` | metrics service and CLI |
| `/usr/lib/qml/TagOS/...` | QML modules (shell, design system, app modules) |
| `/usr/lib/systemd/user/` | `tag-flow.service`, `tag-shell.service`, `tag-session.target` |
| `/usr/lib/systemd/system/tagos-boot-marker.service` | boot-test marker (used by the ISO test) |
| `/usr/lib/environment.d/90-tagos.conf` | session environment defaults |
| `/usr/share/wayland-sessions/tagos.desktop` | the session display managers list |
| `/usr/share/applications/*.desktop` | application entries |
| `/usr/share/icons/hicolor/scalable/apps/` | application icons |
| `/etc/xdg/labwc/` | compositor defaults (rc.xml, autostart, environment, menu, theme) |
| `/etc/greetd/config.toml` | greeter configuration |
| `/usr/share/tagos/themes/` | wallpapers and icon themes |

## Runtime dependencies

`labwc`, `qt6-{base,declarative,svg,wayland}`, `mesa`, `libinput`,
`networkmanager`, `bluez`, `bluez-utils`, `pipewire`, `wireplumber`,
`alsa-utils`, `upower`, `power-profiles-daemon`, `polkit`, `glib2`,
`xdg-desktop-portal{,-wlr}`, `xdg-utils`, `brightnessctl`, `gammastep`, `foot`,
`ttf-liberation`, `inter-font`.

Optional: `greetd` + `greetd-tuigreet` (display manager), `layer-shell-qt`
(layer-shell surfaces in Phase C).

## What the package deliberately does not do

* It does not enable or start system units. Power actions go through
  logind/polkit when the user asks for them.
* It does not ship a display manager. Any Wayland-capable greeter can list
  `tagos.desktop`; greetd is only an `optdepends`.
* It does not touch `/etc/passwd`, `/etc/shadow` or home directories.
* It does not run anything as root. The session and every TagOS service run as
  the logged-in user (`tag-shell.service`, `tag-flow.service` are user units).

## Verification performed on the package

Inside CI (`package-arch` job) the built `.pkg.tar.zst` is unpacked and asserted
to contain the session entry, the user units, the hicolor icons and the `/etc`
configuration, and `.PKGINFO` is printed. A local build additionally runs the
full `ctest` suite through `check()`.

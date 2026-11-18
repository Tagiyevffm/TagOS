# TagOS architecture

The whole point of this layout is that a layer can be replaced without touching
the layers below or above it: swapping labwc for a custom compositor must not
touch systemd units, and changing the shell must not touch the kernel, session
manager or the metrics service.

## Layers

| Layer | Owns | Code |
| --- | --- | --- |
| Hardware / VM | QEMU UEFI, virtio-gpu, virtio-blk, virtio-net | `tools/run-qemu.sh` |
| System foundation | kernel, systemd, Mesa, PipeWire, NetworkManager, BlueZ, D-Bus | Arch packages + `system/` |
| Platform | Wayland session, labwc windows/input/outputs | `system/config/labwc`, `core/tag-session` |
| Framework | Qt 6, QML, C++20, QtDBus | `core/tag-system`, `core/tagflow` |
| Application/services | TagFlow contract, TagMonitor, future apps | `core/tagflow/daemon`, `apps/` |
| User experience | board, bars, dock, launcher, control centre | `desktop/tagshell`, `desktop/tag-design` |

## Module contracts

```
┌──────────────────────────── tag-shell / tag-monitor (QML) ─────────────┐
│  import TagOS.Design 1.0   → Theme tokens + components                 │
│  import TagOS.Flow 1.0     → FlowClient (CPU/RAM/disk/uptime/history)  │
│  import TagOS.System 1.0   → AppLauncher, PowerManager                 │
└───────────────▲───────────────────────────────▲────────────────────────┘
                │ QML types (no headers leaked) │
┌───────────────┴──────────────┐  ┌─────────────┴───────────────────────┐
│ libtagos-flow                │  │ libtagos-system                     │
│  FlowClient                  │  │  sampleSystem(), ProcFs, sampler,   │
│  ├─ LocalBackend  (procfs)   │  │  OsRelease, Format, codecs          │
│  └─ DbusBackend   (QtDBus)   │  │  (pure, fixture-testable)           │
└───────────────▲──────────────┘  └─────────────────────────────────────┘
                │ org.tagos.Flow1 (session bus, a{sv} snapshot)
┌───────────────┴──────────────┐
│ tag-flowd (user service)     │
│  FlowService + tagos-system  │
└──────────────────────────────┘
```

Rules enforced by review and by tests:

* `tag-system` has no UI, no D-Bus and no QML. It is the only place that reads
  `/proc`, `statvfs`, `GetSystemTimes`, and it accepts an injectable root, so
  parsers are tested against fixture files.
* `tagflow` depends on `tag-system` but never on a shell or an app.
* Apps depend on modules, never on each other; there is no app-to-app include.
* The wire contract lives in `core/tagflow/dbus/org.tagos.Flow1.xml`. Adding a
  key is compatible; removing or retyping one requires `org.tagos.Flow2`.

## Two transports, one client

`FlowClient` asks `makePreferredBackend()` at construction:

1. Is `org.tagos.Flow` on the session bus and does `Ping` answer? → `DbusBackend`.
2. Otherwise, or if the service dies later → `LocalBackend` samples in-process.

The UI never knows the difference beyond `transportLabel`, which the monitor
card displays. This is what makes the desktop work on a Windows dev host, and it
is also why a `tag-flowd` crash cannot blank the shell.

## Session boot flow

```
firmware (OVMF)
  └─ systemd default.target
       ├─ NetworkManager, bluetooth, timesyncd
       ├─ greetd
       │    └─ tag-session (live media autologin, or tuigreet on installs)
       │         ├─ exports XDG_*/QT_QPA_PLATFORM=wayland
       │         ├─ dbus-update-activation-environment
       │         └─ exec labwc
       │              └─ /etc/xdg/labwc/autostart
       │                   └─ systemctl --user start tag-session.target
       │                        ├─ tag-flow.service  (tag-flowd)
       │                        └─ tag-shell.service (tag-shell --ready-file %t/tagos/ready)
       └─ tagos-boot-marker.service (live/CI only)
            └─ waits for /run/user/*/tagos/ready → prints TAGOS_BOOT_OK on ttyS0
```

The readiness file is written by `tag-shell` itself, so the boot test proves
that a compositor session, D-Bus, the shell and QML all came up — not just that
the kernel found a root filesystem.

## Design system

`TagOS.Design` owns every visual decision:

```
Theme.qml  → color.*, spacing.*, radius.*, font.*, motion.*, wallpaper, icon()
components → TagButton, TagCard, TagToggle, TagSlider, TagRingGauge, TagSparkline, TagIcon
assets     → themes/icons/*.svg, themes/wallpapers/*.svg (compiled into the module)
```

A component never hardcodes a colour. Changing the accent token re-themes the
shell, the monitor and every future app at once.

## Security model

| Process | Runs as | Sandbox |
| --- | --- | --- |
| `tag-flowd` | session user | `NoNewPrivileges`, `ProtectSystem=strict`, `ProtectHome`, empty capabilities, `RestrictAddressFamilies=AF_UNIX`, syscall filter `@system-service` |
| `tag-shell` | session user | `NoNewPrivileges`, `ProtectSystem=strict`, read-only home |
| power actions | session user | delegated to logind via polkit; live media ships a narrowly-scoped rule, installed systems prompt |
| live media | `tagos` (sysusers) | no hand-written `/etc/passwd`, no root desktop session |

No TagOS service runs as root. `PowerManager` and `AppLauncher` never shell out
with unquoted strings and `AppLauncher` resolves executables through `PATH`
before spawning, so a missing application produces a visible error instead of a
silent no-op.

## Shell surface model

The shell is one frameless, full-screen `Window` ("board") that hosts the merged
composition: wallpaper, desktop icons, clock widget, monitor card, and the
chrome drawn on top (panel, dock, overlays). The chrome is not built into the
window — it lives in discrete content components:

```
Main.qml (Window)
├── BoardContent        wallpaper · desktop icons · Hero · MonitorCard · menu
├── TopPanelContent     brand · quick launch · global search · tray · clock
├── DockContent         launcher · pinned apps · all-apps entry · context menu
├── LauncherContent     Ctrl+Space search, keyboard driven
├── ControlCenterContent tiles · sliders · battery · notifications
└── NotificationStack   toasts for real events
```

Each `*Content.qml` is an `Item` with no window assumptions, so promoting the
panel and dock to `wlr-layer-shell` surfaces (exclusive zones, per-output
anchors) is a change to the window wrappers only. `layer-shell-qt` is already in
`system/packages/desktop.txt`. This is the Phase 2 step: it needs a Wayland host
to verify that exclusive zones actually reserve space, and the Windows
development host used for this phase cannot prove that.

## Settings flow

`TagOS.Config` owns one JSON document (`~/.config/tagos/settings.json`):

```
TagSettings ─┐
TagCenter  ──┼─→ SettingsStore (singleton) ─→ 250 ms debounce ─→ QSaveFile (atomic)
Dock menu  ──┘                 │
                               ├─→ Theme (dark/light palette) → every component
                               ├─→ dock size · auto-hide · panel height
                               └─→ pinned apps · recent apps
```

A corrupt file falls back to defaults and surfaces a Turkish error string in the
shell notification centre instead of failing to start. Values are validated on
write (theme ∈ dark|light|auto, icon size 28–64, panel height 32–72, …) so a bad
setting can never render an unusable desktop.

## Deliberate Phase 1 limits

* The shell is still **one composited surface**; see the surface model above for
  what promotion to layer-shell requires and why it was not claimed here.
* Positions are computed for a single output. Multi-monitor generation is a
  wrapper change plus `Screen` iteration, deferred to a Wayland host.
* Network/audio/Bluetooth/power tiles are backed by real CLI adapters
  (`nmcli`, `wpctl`, `bluetoothctl`, `upower`, `powerprofilesctl`,
  `brightnessctl`, `rfkill`, `gammastep`). When a tool is missing the tile is
  disabled and says why — there are no decorative switches.
* TagTerminal, the store, updates and snapshots are later phases. The launcher
  only lists applications that actually exist on the system.

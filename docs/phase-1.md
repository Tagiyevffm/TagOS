# Phase 1 — Desktop (v0.2.0 «Aurora»)

Phase 1 replaced the Phase 0 proof-of-concept board with a real desktop layer:
the seven design tokens became a full theme with light and dark palettes, the
shell gained a launcher, a control centre and notifications, and three new
applications plus four new core modules were added.

Nothing in this phase was simulated. Where a system service was not available on
the development host, the surface says so and disables the control instead of
showing a switch that does nothing.

## What shipped

| Area | Delivered | Verified on this host |
| --- | --- | --- |
| Design system | `Theme` (dark + light palettes, 60 tokens), `Spacing`, `Radius`, `Typography`, `Motion`, 11 components, 78 icons × 2 themes, 2 wallpapers | rendered (screenshots) |
| Shell | Board, top panel, dock, launcher, control centre, toasts, context menus, keyboard shortcuts | `desktop-shell-smoke` + screenshots |
| Settings | `TagOS.Config` (`~/.config/tagos/settings.json`, debounced atomic save, validation, bounds) | `tst_settings_store` (10 cases) |
| Applications | `TagApps` catalog, TagFiles, TagSettings, TagMonitor (rewritten), TagAbout | 4 apps boot and render |
| System control | `SystemControl`: NetworkManager, BlueZ, PipeWire, UPower, power-profiles-daemon, brightnessctl, rfkill, gammastep | parsing covered by 16 unit cases; live backends require Linux |
| Files | `TagOS.Files`: directory model, copy/move/rename/trash/permanent delete, bounded search, previews | `tst_filesystem` (17 cases) |
| Monitors | `ProcessList` (/proc, delta CPU), `NetworkSampler` (/proc/net/dev), `ThermalReader` | `tst_monitors` (8 cases) |
| Runtime | `tag-runtime`: one bootstrap for every GUI binary (`--smoke-test`, `--screenshot`, `--ready-file`, logging, QML error counting) | all apps use it |

## Acceptance checks

| # | Criterion | Result |
| --- | --- | --- |
| 1 | Dark and light themes driven by one settings file | pass — `board-dark.png`, `board-light.png` |
| 2 | Launcher searches real installed applications | pass — 4 desktop entries discovered via XDG dirs |
| 3 | Dock pins/unpins and persists | pass — `tst_settings_store::pinsAreUnique`, dock menu |
| 4 | Control centre reflects real state | pass (honest) — every tile reports *desteklenmiyor* when the service is absent |
| 5 | Notifications exist and can be dismissed | pass — in-process centre, toasts on real events |
| 6 | TagFiles performs real mutations | pass — 17 filesystem tests plus live home directory |
| 7 | TagSettings writes real settings | pass — `tst_settings_store` + live UI |
| 8 | TagMonitor shows processes/network/thermal | pass on Linux; on the Windows dev host `/proc` is absent and the UI states it |
| 9 | No QML runtime errors | pass — smoke test fails on any `TypeError`/`is not a type` |
| 10 | Everything builds and tests green | pass — 13/13 CTest |

## Deliberately not claimed

* **Layer-shell surfaces.** The shell still draws its chrome inside one
  composited board window. The panel, dock and control centre are discrete
  `*Content.qml` components so promoting them to `wlr-layer-shell` surfaces is a
  window-wrapper change, but this host has no Wayland session to verify
  exclusive zones, so the split-surface mode was not enabled — a half-tested
  layer-shell surface would reserve screen space it cannot prove.
* **Multi-monitor.** Same reason: positions are computed from the board size and
  are correct for one screen; per-screen generation needs a Wayland host.
* **Real network/audio toggles on this host.** `nmcli`, `wpctl`, `bluetoothctl`
  and `upower` do not exist on Windows; the adapters and their parsers are
  tested, the IPC path is not exercised here.
* **TagTerminal.** Not built in this phase. A terminal that is not a PTY-backed
  emulator would be a fake window; the plan is a `foot`/`VTE`-class backend,
  tracked in the roadmap.
* **updates, snapshots, install-time partitioning.** Phases 4–5.

## Where to look

* `docs/architecture.md` — module layers, surface model, settings flow.
* `docs/modules.md` — module table with interfaces and status.
* `docs/testing.md` — how to run every check, including the QEMU boot test.

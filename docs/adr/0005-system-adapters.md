# ADR 0005 — Integrate existing system services behind TagOS adapters

* Status: accepted (Phase 1)
* Date: 2025-11
* Context: TagCenter and TagSettings need Wi-Fi, Bluetooth, volume, brightness,
  battery, power profiles, night light and airplane mode. Writing NetworkManager,
  BlueZ or PipeWire clients from scratch would be weeks of work, would duplicate
  battle-tested policy handling (secrets, profiles, policy routing) and would put
  TagOS in the position of re-implementing bugs.

## Decision

TagOS integrates the existing services and owns only the presentation layer.
`TagOS.Control` (`SystemControl`) drives:

| Capability | Backend | Why |
| --- | --- | --- |
| Wi-Fi | `nmcli` (NetworkManager) | NM owns profiles, secrets, roaming |
| Bluetooth | `bluetoothctl` (BlueZ) | BlueZ owns pairing and the adapter state |
| Volume / mute | `wpctl` (PipeWire + WirePlumber) | the session's real default sink |
| Brightness | `brightnessctl` | backlight class, no root daemon needed |
| Battery | `upower` | UPower already aggregates every power supply |
| Power profiles | `powerprofilesctl` | platform-profile driver selection |
| Night light | `gammastep` | colour temperature, no compositor support required |
| Airplane mode | `rfkill` | kernel-level radio blocking |

Rules that keep this honest:

1. Availability is probed once at start-up. A missing tool makes the capability
   permanently `available == false`; the UI shows a disabled tile with the reason
   instead of a switch that does nothing.
2. Output parsing lives in `ControlParsers` as pure functions with unit tests, so
   a CLI format change fails a test rather than silently reporting the wrong
   battery percentage.
3. No service is re-implemented and nothing is written outside the user's own
   session; privileged actions go through logind/polkit.
4. The adapters are the in-process precursor of the Phase 2 D-Bus services
   (`org.tagos.Network1`, `org.tagos.Audio1`, …). When those land, the QML keeps
   the same property names and only the transport changes.

## Consequences

* TagOS ships real functionality on day one and inherits upstream fixes.
* The dependency list grows (`system/packages/desktop.txt`), and a stripped
  system loses those tiles — visibly, not silently.
* CLI calls are asynchronous and coalesced (slider drags become one `wpctl`
  call), so the UI never blocks on a subprocess.
* Polling costs a few short-lived processes every 15 s; acceptable, and the
  Phase 2 D-Bus services will replace polling with signals.

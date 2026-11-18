# Testing

Every check below is part of the repository and runs the same way locally and in
CI. Nothing is a screenshot-only claim: a test either passes or the build fails.

## Test inventory

| CTest target | Type | What it proves |
| --- | --- | --- |
| `tst_procfs` | unit | `/proc` readers with fixture files (stat, meminfo, uptime, loadavg) |
| `tst_cpu_sampler` | unit | CPU usage deltas, multi-sample history, counter wrap |
| `tst_format` | unit | byte/percent/uptime/load formatting shared by CLI and UI |
| `tst_osrelease` | unit | `os-release` parsing and display-name fallbacks |
| `tst_metrics_codec` | unit | TagFlow wire encoding round-trip (`schema=1`) |
| `tst_flow_wire` | unit | message framing and transport payloads |
| `tst_settings_store` | unit | defaults, persistence, validation, clamping, recents/pins, corrupt-file fallback, atomic save |
| `tst_desktop_entry` | unit | `.desktop` parsing, field-code expansion, quoting, locale keys |
| `tst_control_parsers` | unit | `nmcli`, `wpctl`, `bluetoothctl`, `upower`, `brightnessctl`, `powerprofilesctl`, `rfkill` output parsing |
| `tst_trash` | unit | FreeDesktop Trash: `.trashinfo` encoding, collision suffixes, restore without overwrite, empty, symlinks preserved (POSIX) |
| `tst_filesystem` | unit | directory model, sorting, hidden files, copy/move/rename, recursive delete, search, previews |
| `tst_monitors` | unit | `/proc`-based process CPU deltas, network totals, thermal scaling |
| `desktop-shell-smoke` | QML smoke | loads the whole shell offscreen; fails on any QML runtime error |
| `repo-system-units` | repository | systemd units, labwc/greetd config, package lists, archiso profile, icon references |
| `tagos-boot-test` | integration (Linux) | builds the ISO, boots it under OVMF, waits for `TAGOS_BOOT_OK` |

The app smoke tests can be run by hand for every GUI binary:

```bash
for app in tag-shell tag-files tag-settings tag-monitor tag-about; do
    QT_QPA_PLATFORM=offscreen ./build/bin/$app --smoke-test || echo "$app FAILED"
done
```

## Running the suite

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

On a Windows development host use MSYS2/UCRT64 (`C:\msys64\ucrt64\bin` on
`PATH`) and `build/win`; the same 14 tests run, minus the boot test.

`tools/lint.sh` adds `qmllint` over every QML file, a hex-colour check (colours
may only appear in `Theme.qml`) and a trailing-whitespace check.

## What each layer does and does not cover

* **Unit tests** cover parsing and state logic with fixtures, so a `/proc` or CLI
  format change fails a test instead of silently reporting wrong numbers.
* **QML smoke tests** load the real UI and fail on `TypeError`, missing types and
  failed bindings — the class of bug a "did it load?" check misses.
* **Repository tests** keep the Linux-side artefacts honest: a unit file that
  references a missing binary, a `.desktop` entry pointing at a nonexistent icon
  or an archiso profile without a boot entry fails.
* **Integration** (`tests/integration/boot-test.sh`) requires an Arch Linux host
  with `qemu-full`, `edk2-ovmf` and `archiso`. It is the only check that proves
  the ISO boots, the session starts and the shell reaches a Wayland surface.

## Development helpers

`TAGOS_DRY_RUN=1` turns power actions and application launches into recorded
no-ops so UI tests never reboot the machine or spawn windows.
`TAGOS_CONFIG_HOME=<dir>` points the settings store at a scratch directory, which
is how the light-theme screenshot is produced without touching real settings.
`TAGOS_SHELL_STATE=launcher|center` opens a shell surface right after load for
screenshots.

```bash
# dark board, launcher, control centre
TAGOS_SHELL_STATE=launcher QT_QPA_PLATFORM=offscreen ./build/bin/tag-shell --screenshot out/launcher.png
# light theme from a scratch settings file
printf '{"themeMode":"light"}' > out/cfg/settings.json
TAGOS_CONFIG_HOME=$PWD/out/cfg QT_QPA_PLATFORM=offscreen ./build/bin/tag-shell --screenshot out/light.png
```

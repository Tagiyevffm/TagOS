# Modules

Every module owns its sources, its tests and (conceptually) its version. The
monorepo shares one version number for Phase 0; `tagos_set_module()` in
`cmake/TagOSUtils.cmake` stamps per-module versions so they can diverge later.

| Module | Path | Type | Status | Interface |
| --- | --- | --- | --- | --- |
| `tag-common` | `core/tag-common` | headers only | Phase 1 ✅ | `tag-common/Export.h` (`TAGOS_EXPORT`) |
| `tag-system` | `core/tag-system` | static lib + `tagos-metrics` CLI | Phase 0 ✅ | `ProcFs`, `CpuUsageSampler`, `OsRelease`, `Metrics`, `Format` |
| `TagOS.System` | `core/tag-system` | QML module (shared) | Phase 1 ✅ | `AppLauncher`, `PowerManager`, `SessionInfo`, `ProcessList`, `NetworkSampler`, `ThermalReader` |
| `tag-flow` | `core/tagflow` | shared lib (QML module `TagOS.Flow`) | Phase 0 ✅ | `FlowClient`, `Backend`, wire codec |
| `tag-flowd` | `core/tagflow/daemon` | user service | Phase 0 ✅ (Linux) | `org.tagos.Flow1` on the session bus |
| `tag-config` | `core/tag-config` | shared lib (QML module `TagOS.Config`) | Phase 1 ✅ | `SettingsStore` singleton, `~/.config/tagos/settings.json` |
| `tag-apps` | `core/tag-apps` | shared lib (QML module `TagOS.Apps`) | Phase 1 ✅ | `AppCatalog`, `parseDesktopEntry`, `expandExecFieldCodes` |
| `tag-control` | `core/tag-control` | shared lib (QML module `TagOS.Control`) | Phase 1 ✅ | `SystemControl`, `ControlParsers` |
| `tag-fs` | `core/tag-fs` | shared lib (QML module `TagOS.Files`) | Phase 1 ✅ | `DirModel`, `FileOps`, `FileSearch`, `Preview`, `PathUtils` |
| `tag-runtime` | `core/tag-runtime` | static lib | Phase 1 ✅ | `TagOS::Runtime::run()`, shared app bootstrap |
| `tag-session` | `core/tag-session` | POSIX shell | Phase 0 ✅ | greetd session command |
| `TagOS.Design` | `desktop/tag-design` | QML module (shared) | Phase 1 ✅ | `Theme`/`Spacing`/`Radius`/`Typography`/`Motion` + 11 components |
| `tag-shell` | `desktop/tagshell` | executable (QML module `TagOS.Shell`) | Phase 1 ✅ | `--ready-file`, `--smoke-test`, `--screenshot`, `TAGOS_SHELL_STATE` |
| `tag-files` | `apps/tag-files` | executable (QML module `TagOS.FilesApp`) | Phase 1 ✅ | desktop entry, `--smoke-test` |
| `tag-settings` | `apps/tag-settings` | executable (QML module `TagOS.Settings`) | Phase 1 ✅ | desktop entry, 6 pages |
| `tag-monitor` | `apps/tag-monitor` | executable (QML module `TagOS.Monitor`) | Phase 1 ✅ | 3 tabs: Genel / Süreçler / Ağ |
| `tag-about` | `apps/tag-about` | executable (QML module `TagOS.About`) | Phase 1 ✅ | desktop entry |
| `tag-terminal` | `apps/tag-terminal` | executable | Phase 2 | planned (PTY-backed emulator) |
| `tag-store` | `apps/tag-store` | executable | Phase 4 | planned |
| `tag-update` | `services/tag-update` | service + UI | Phase 4 | planned (`org.tagos.Update1`) |
| `tag-snapshot` | `services/tag-snapshot` | service | Phase 4 | planned (`org.tagos.Snapshot1`) |
| `tag-power` | `core/tag-power` | service | Phase 2 | planned (`org.tagos.Power1`) |
| `tag-network` | `services/tag-network` | NM/BlueZ adapter | Phase 2 | `SystemControl` is the in-process precursor |
| `tag-audio` | `services/tag-audio` | PipeWire adapter | Phase 2 | `SystemControl` is the in-process precursor |
| `tag-notify` | `services/tag-notify` | notification daemon | Phase 2 | `org.freedesktop.Notifications`; shell currently owns an in-memory centre |
| `tag-ai` | `services/tag-ai` | optional service | Phase 4 | planned |

## Adding a module

1. Create `<area>/<module>/` with `CMakeLists.txt`, `include/`, `src/`, `README.md`.
2. If it exposes C++: static library with a narrow `include/` tree.
3. If it exposes QML: `qt_add_library(... SHARED)` + `qt_add_qml_module(URI TagOS.<Name>)`
   and an export header (see `core/tagflow/include/tagflow/FlowExport.h`).
4. Add unit tests under `tests/unit/` and register them in `tests/unit/CMakeLists.txt`.
5. If it is a service: a unit in `system/systemd/user/`, sandboxed, user-level.
6. If it needs IPC: a contract XML in the module's `dbus/` directory and a
   matching adaptor — the XML is the source of truth for compatibility.

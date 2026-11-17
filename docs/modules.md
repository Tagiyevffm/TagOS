# Modules

Every module owns its sources, its tests and (conceptually) its version. The
monorepo shares one version number for Phase 0; `tagos_set_module()` in
`cmake/TagOSUtils.cmake` stamps per-module versions so they can diverge later.

| Module | Path | Type | Status | Interface |
| --- | --- | --- | --- | --- |
| `tag-system` | `core/tag-system` | static lib + `tagos-metrics` CLI | Phase 0 ✅ | C++20 headers under `include/tag-system/` |
| `TagOS.System` | `core/tag-system` | QML module (shared) | Phase 0 ✅ | `AppLauncher`, `PowerManager` |
| `tag-flow` | `core/tagflow` | shared lib (QML module `TagOS.Flow`) | Phase 0 ✅ | `FlowClient`, `Backend`, wire codec |
| `tag-flowd` | `core/tagflow/daemon` | user service | Phase 0 ✅ (Linux) | `org.tagos.Flow1` on the session bus |
| `tag-session` | `core/tag-session` | POSIX shell | Phase 0 ✅ | greetd session command |
| `TagOS.Design` | `desktop/tag-design` | QML module (shared) | Phase 0 ✅ | `Theme` singleton + components |
| `tag-shell` | `desktop/tagshell` | executable (QML module `TagOS.Shell`) | Phase 0 ✅ | `--ready-file`, `--smoke-test` |
| `tag-monitor` | `apps/tag-monitor` | executable (QML module `TagOS.Monitor`) | Phase 0 ✅ | desktop entry, no CLI flags |
| `tag-files` | `apps/tag-files` | executable | Phase 3 | planned |
| `tag-settings` | `apps/tag-settings` | executable | Phase 3 | planned |
| `tag-store` | `apps/tag-store` | executable | Phase 4 | planned |
| `tag-update` | `services/tag-update` | service + UI | Phase 4 | planned (`org.tagos.Update1`) |
| `tag-snapshot` | `services/tag-snapshot` | service | Phase 4 | planned (`org.tagos.Snapshot1`) |
| `tag-power` | `core/tag-power` | service | Phase 2 | planned (`org.tagos.Power1`) |
| `tag-network` | `services/tag-network` | NM/BlueZ adapter | Phase 2 | planned (`org.tagos.Network1`) |
| `tag-audio` | `services/tag-audio` | PipeWire adapter | Phase 2 | planned (`org.tagos.Audio1`) |
| `tag-notify` | `services/tag-notify` | notification daemon | Phase 2 | `org.freedesktop.Notifications` |
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

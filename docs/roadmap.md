# TagOS roadmap

Each phase must be verifiable before the next starts: a phase is done when its
tests pass, not when its UI looks finished.

| Phase | Scope | Exit criteria | Status |
| --- | --- | --- | --- |
| **A — Arch Linux foundation** | module audit, session entry, install layout (`/usr` + `/etc`), native trash, PKGBUILD, Linux tooling, Arch CI | tree builds and tests on Arch, package payload verified, install tree asserted | code complete; CI jobs committed, not yet observed green |
| **0 — Foundation** | Arch base, UEFI/QEMU, systemd user session, labwc, Qt6/QML shell, metrics, tests | `ctest` green, ISO boots to a live TagOS session, boot marker seen on serial | build/tests green; ISO boot still unverified (needs a Linux host) |
| **1 — TagShell / desktop** ✅ v0.2.0 | themed shell (panel, dock, launcher, control centre), settings store, TagFiles/TagSettings/TagMonitor/TagAbout | theme switching, application launch, settings persistence, 13 tests | delivered on Windows; Linux-side behaviour unverified |
| **B — Native system integration** | NetworkManager/BlueZ/UPower/PipeWire/power-profiles-daemon D-Bus services replacing CLI polling; settings migration; event-driven `tag-flowd` | QML receives signals, no CLI polling left in the hot path, no root daemon added | next |
| **2 — System integration** | `tag-network`, `tag-audio`, `tag-bluetooth` adapters over NetworkManager/BlueZ/PipeWire; notifications (`tag-notify`); battery/backlight (`tag-power`) | TagCenter tiles control real radios/volume; notifications survive shell restart; no root daemon added |
| **3 — Core applications** | TagFiles, TagSettings, TagMonitor (full), TagTerminal, TagAbout; `.desktop` catalogue + recent files | every launcher tile launches a real app; TagSettings edits the same keys the system reads |
| **4 — Advanced features** | TagSnapshot (btrfs), TagUpdate (pacman transaction staging), TagStore (Flatpak + pacman), TagAI Core, multiple workspaces | update staging never touches a running session; snapshot rollback tested in the VM |
| **5 — Installer** | archiso install wizard: disk partitioning, user creation, locale, bootloader, first-boot setup | installed system boots without the medium; installer tested on BIOS-less UEFI, virtio and SATA |
| **6 — Optimization** | boot time, RAM/CPU budget, GPU acceleration (virtio + real hardware), crash and suspend/resume tests | documented before/after numbers; 3 GiB VM stays responsive under load |
| **7 — v1.0** | full-system test matrix, ISO signing, update mechanism, documentation, release | reproducible ISO, signed checksums, upgrade path from v0.x |

## Where verification stands

The development machine used for Phases 0–1 and for the Arch migration has no
working Linux runtime, so **no Wayland, D-Bus-session, layer-shell, ISO-boot or
installer result has been observed yet**. Everything claimed as working is either
a Windows-verified build/test or a committed CI definition. See
[ARCH_LINUX_MIGRATION.md](ARCH_LINUX_MIGRATION.md) for the environment evidence and
the per-gap schedule; the first action for the next phase is a green CI run, not
new features.

## Phase C notes (layer-shell, next)

* `wlr-layer-shell`: Phase 0's single surface becomes
  `tag-shell --panel=top|dock`, each a layer-shell surface. `layer-shell-qt` is
  already in `system/packages/desktop.txt`.
* Workspace model moves out of `TopBar.qml` into a `WorkspaceModel` in
  `tagflow`, sourced from labwc's `foreign-toplevel` protocol.
* Theme switching: `Theme` becomes data-driven (JSON palettes in `themes/`),
  with `TagOS.Design` reading the active palette at runtime.

## Standing risks

| Risk | Mitigation |
| --- | --- |
| Wayland + QML panels need layer-shell | Phase 1 uses `layer-shell-qt`; a custom surface is only considered after the protocol path is proven |
| Rolling-release Arch packages break builds | CI builds against current `extra`; ISO builds pin a package snapshot file |
| A TagOS service crash takes the desktop down | every service is a separate systemd unit, clients degrade (FlowClient falls back to local sampling) |
| GPU support in VMs vs. hardware | virtio-gpu in QEMU, real-hardware matrix in Phase 6 |
| Root creep in services | user units only; `NoNewPrivileges` + capability-free sandboxing checked by `repo-system-units` |

# ADR 0004 — Settings live in one validated JSON document

* Status: accepted (Phase 1)
* Date: 2025-11
* Context: the shell, TagFiles, TagSettings, TagCenter and the dock menu all need
  the same appearance and shell state (theme, wallpaper, dock size/auto-hide,
  pinned and recent apps). Phase 0 kept everything in QML properties, so a
  restart reset it and each surface could disagree.

## Decision

`TagOS.Config` (`SettingsStore`) owns exactly one document at
`$XDG_CONFIG_HOME/tagos/settings.json` (overridable with `TAGOS_CONFIG_HOME`).

* Writes are debounced by 250 ms and applied with `QSaveFile`, so a crash mid-save
  cannot leave a truncated file.
* Reads are validated: unknown enum values fall back to defaults, numeric values
  are clamped to the UI's real range, and a corrupt file yields defaults plus a
  visible Turkish error instead of a broken desktop.
* The store exposes typed properties and setters; QML never touches JSON.
* Pinned applications are a list of desktop-entry ids, so the dock, desktop and
  TagSettings resolve the same application through `TagOS.Apps`.

## Consequences

* One file to back up, inspect or reset; no dconf/GSettings dependency.
* Adding a setting is a property + a validation rule + a row in TagSettings.
* Two processes writing at once can still lose one update (last write wins).
  Acceptable while only one shell instance exists; a file lock or a small D-Bus
  settings service would be the fix if settings ever need to be shared by
  multiple live writers.
* Per-user only. System-wide defaults are configuration shipped in
  `system/config`, not entries in this file.

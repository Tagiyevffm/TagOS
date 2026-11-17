# ADR 0001 — Arch Linux base, labwc compositor, Qt 6/QML desktop

* Status: accepted
* Date: 2026-11-17 (Phase 0)

## Context

TagOS must be a distribution with its own desktop environment, not a themed
install of someone else's desktop. Building a compositor from scratch in the
first release would put the project months away from a bootable system.

## Decision

* **Base:** Arch Linux packages (rolling), because TagOS needs current Qt,
  Wayland and Mesa without backporting.
* **Compositor:** `labwc` — wlroots-based, Openbox-like configuration, small
  enough to configure rather than fork. TagOS ships configuration and a
  layer-shell-capable shell around it.
* **Toolkit:** Qt 6 / QML with C++20 for system integration.
* **Custom compositor:** explicitly out of scope until after v1.0.

## Consequences

* We inherit Arch's rolling-release risk: package bumps can break builds. CI
  builds against current `extra` on every change, and ISO builds pin a package
  snapshot.
* We get Wayland, fractional-scale-ready rendering, and a session that boots in
  a 3 GiB VM without writing a compositor.
* The layer-shell boundary is real: Phase 0's shell is a single surface, and
  Phase 1 splits the bars into layer-shell surfaces. Nothing above the shell
  needs to change for that.

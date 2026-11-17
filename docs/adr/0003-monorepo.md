# ADR 0003 — One monorepo, per-module tests and versions

* Status: accepted
* Date: 2026-11-17 (Phase 0)

## Context

A distribution spans a session script, systemd units, an archiso profile, QML
components and C++ libraries. Splitting these into repositories would make
"does this combination boot?" a cross-repo build problem before the project has
a single release.

## Decision

* One repository, one CMake build, one CTest run.
* Every module still owns its directory, its public headers, its tests and its
  documentation, and is stamped with its own version through
  `tagos_set_module()`.
* Repository-level integration is checked by a plain-Python test
  (`tests/system/test_system_files.py`) instead of by a CI-only script, so the
  same checks run locally, in CI and before an ISO build.

## Consequences

* `cmake --build` + `ctest` verifies the C++/QML half of the distribution on any
  machine with a Qt 6 toolchain, including a Windows workstation.
* Unit tests cannot silently depend on a live machine: parsers take fixture
  paths, and desktop tests run offscreen with `TAGOS_DRY_RUN=1`.
* When a module needs independent release cadence (Phase 4 services), the
  per-module version stamps already exist; only the packaging split remains.

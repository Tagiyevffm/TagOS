# ADR 0002 — D-Bus as the TagOS service boundary

* Status: accepted
* Date: 2026-11-17 (Phase 0)

## Context

TagOS modules must be replaceable and independently crashable. Linking the
shell against every service's C++ headers would couple their ABI and let one
service crash take the desktop with it. The plan calls this layer TagFlow.

## Decision

* Cross-process communication uses the session D-Bus with `org.tagos.*`
  interfaces, versioned in the name (`org.tagos.Flow1`).
* Each interface has a contract XML in the owning module's `dbus/` directory;
  the XML, not the implementation, defines compatibility.
* Payloads are `a{sv}` maps with a `schema` version, so additive changes do not
  break older clients and an unknown schema is rejected explicitly.
* Clients must degrade: `FlowClient` falls back to in-process sampling when the
  service is missing or dies. The UI never shows a permanent blank state.

## Consequences

* TagFlow's wire codec is unit tested (`tst_flow_wire`) and shared by both the
  service and the client, so both sides cannot drift silently.
* Windows developer builds have no QtDBus; the local backend keeps the UI
  developable. This is a development affordance, not a shipped configuration.
* Extra indirection (serialize → bus → deserialize) for a 20-field snapshot is
  acceptable: one 1.5 s poll costs microseconds, and the isolation is the point.

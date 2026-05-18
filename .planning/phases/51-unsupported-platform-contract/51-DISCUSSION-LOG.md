# Phase 51: Unsupported-Platform Contract - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-18
**Phase:** 51-unsupported-platform-contract
**Areas discussed:** Unsupported placeholder presentation, Imperative API contract, Auto-start and property changes, Warning precedence and diagnostic detail

---

## Unsupported placeholder presentation

| Option | Description | Selected |
|--------|-------------|----------|
| Same core message in both; `sealed` minimal and `diagnostic` adds extra status/detail | Keeps the already-approved mode split while making unsupported behavior visible in both modes | ✓ |
| Show the same detailed unsupported message in both modes | Maximizes consistency, but makes `sealed` heavier than previous phases intended | |
| Keep `sealed` very subtle and reserve the real explanation for `diagnostic` | Risks making the default experience look blank or broken | |

**User's choice:** Same core Windows-only message in both; `sealed` stays minimal and `diagnostic` adds extra status/detail.
**Notes:** The follow-up wording choice was to make the core message explain that the host/control is valid, but embedded mdCAD viewing is Windows-only and will not launch on the current platform.

---

## Imperative API contract

| Option | Description | Selected |
|--------|-------------|----------|
| `StartAsync()` fails immediately with a clear unsupported-runtime error; `StopAsync()` is a safe no-op | Gives programmatic hosts a deterministic contract instead of a silent non-start | ✓ |
| `StartAsync()` silently no-ops and only updates warning/state; `StopAsync()` is a safe no-op | Keeps UI quiet, but leaves imperative callers guessing | |
| `StartAsync()` completes successfully but records unsupported warning/detail state; `StopAsync()` is a safe no-op | Avoids exceptions, but makes launch success misleading | |

**User's choice:** `StartAsync()` fails immediately with a clear unsupported-runtime error; `StopAsync()` is a safe no-op.
**Notes:** The follow-up choice was to keep the failure text aligned with the same truth shown in the placeholder surface rather than using a colder technical error.

---

## Auto-start and property changes

| Option | Description | Selected |
|--------|-------------|----------|
| Show unsupported state as soon as the control attaches; `AutoStart` never launches and property changes update descriptive state only | Tells the truth immediately and avoids hidden runtime attempts | ✓ |
| Ignore `AutoStart` and property changes silently until `StartAsync()` is called | Leaves default hosts looking idle/broken until something explicit happens | |
| Show unsupported state only after an explicit launch attempt, even if `AutoStart=true` | Delays the truth and makes the default configuration misleading | |

**User's choice:** Show unsupported state as soon as the control attaches; `AutoStart` never launches and property changes only update descriptive state.
**Notes:** The follow-up choice was to keep requested JSONL/live-refresh intent visible only as informational diagnostics in `diagnostic` mode.

---

## Warning precedence and diagnostic detail

| Option | Description | Selected |
|--------|-------------|----------|
| Unsupported-platform message stays primary; requested JSONL/live-refresh details may appear only as secondary informational diagnostics | Keeps the real contract visible and avoids misleading Windows-only validation on unsupported hosts | ✓ |
| Show unsupported-platform message and normal JSONL/path warnings side by side with equal weight | More detail, but muddles the primary runtime contract | |
| Let JSONL/path validation warnings take precedence when requested startup settings are invalid | Risks implying the path is the main problem instead of runtime support | |

**User's choice:** The unsupported-platform message stays primary; requested JSONL/live-refresh settings may appear only as secondary informational diagnostics.
**Notes:** The follow-up choice was to disable the control's own `diagnostic` launch action up front on unsupported platforms instead of leaving it clickable.

---

## the agent's Discretion

- Exact unsupported-backend type names, file layout, and backend-selection wiring
- Exact placeholder styling/iconography and layout treatment
- Exact exception type used for unsupported `StartAsync()` failure
- Exact wording/placement of secondary informational diagnostics for requested JSONL/live-refresh intent

## Deferred Ideas

- Public `IsRuntimeSupported`-style capability property
- Host-customizable unsupported placeholder text
- Plain `net10.0` TFM widening and proof-host work
- Dotnet-managed Windows runtime refresh automation
- Consumer proof and docs/onboarding truthfulness follow-up work

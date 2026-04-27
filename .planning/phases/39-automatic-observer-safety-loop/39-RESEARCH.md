# Phase 39: Automatic Observer Safety Loop - Research

**Researched:** 2026-04-27  
**Domain:** Flat JSONL observer auto-refresh safety loop (Phase 39)  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Auto-refresh opt-in lifecycle
- **D-01:** For flat roots, auto-observe starts immediately when link is active and a source path is present.
- **D-02:** Turning `Link file for refresh` OFF stops observing immediately but keeps the stored source path for quick re-enable.
- **D-03:** Flat-root inspector must expose a separate `Observe automatically` toggle next to link state (observe is not implicit-only).
- **D-04:** If observe is ON but source path is empty/invalid, runtime stays idle and surfaces a warning; this state must not consume retry budget.

### Carry-forward safety defaults (from prior phases)
- **D-05:** Keep debounce/retry/auto-disable safety semantics aligned with established observer behavior from Phase 35 unless explicitly overridden by this phase plan.
- **D-06:** Manual `Re-import now` remains available even when automatic observe flow is disabled by safety behavior.
- **D-07:** Automatic safety loop applies to flat observer roots only (non-sketch entities with `JsonlObserverComp`), preserving sketch observer path behavior.

### the agent's Discretion
- Exact inspector microcopy/layout for link/observe/safety status, as long as D-01..D-04 remain explicit.
- Whether interval/retry controls are shown directly in Phase 39 flat inspector or preserved as internal defaults, as long as D-05 behavior is enforced and observable.
- In-flight coalescing policy when file changes during active flat refresh, provided safety and transactional guarantees remain intact.

### Deferred Ideas (OUT OF SCOPE)
- Large-scale repeated-refresh interaction/performance hardening remains Phase 40.
- Any diff/patch incremental refresh strategy remains outside v1.6 scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| OBSF-04 | Automatic observer refresh uses debounce/retry/auto-disable safety behavior for unstable file-write windows. | Reuse `jsonl_observer_source_changed(...)` + sketch retry policy in `jsonl_observer_tick_one(...)`, but route refresh execution through flat transactional slots (`jsonl_observer_request_flat_refresh`/`jsonl_observer_tick_flat_refreshes`). |
</phase_requirements>

## Project Constraints (from copilot-instructions.md)

No `./copilot-instructions.md` found in repository root.  
Applied constraints from request/context: C-first conventions, header-only patterns, preserve Phase 38 manual transactional semantics, avoid sketch pipeline overhead in flat loop.

## Summary

Phase 38 already provides the critical execution primitives needed for Phase 39: transactional flat subtree replacement, in-flight dedupe per root, and inspector/manual refresh controls. The missing piece is an **automatic scheduler** for flat roots that applies Phase 35 safety semantics (debounce/retry/auto-disable) without entering sketch reparse paths.

The safest implementation is to add a dedicated flat-root auto tick in `jsonl_observer_system.h` that: (1) gates on linked + observe-enabled + valid source path, (2) uses `jsonl_observer_source_changed(...)` for change detection, (3) starts refresh only when no slot is running for that root, (4) applies retry/backoff/disable exactly like sketch observer behavior.

This preserves Phase 38 manual refresh contract (manual remains callable even after auto-disable), keeps transactional replacement semantics unchanged, and avoids sketch pipeline overhead.

**Primary recommendation:** Implement `jsonl_observer_tick_one_flat(...)` + extend `jsonl_observer_system_tick(...)` query to flat `JsonlObserverComp` roots (non-sketch), while leaving existing sketch tick and flat manual execution functions intact.

## Existing Architecture Findings and Reusable Assets

- `jsonl_observer_request_flat_refresh(...)` already enforces one in-flight refresh per root and queues transactional job start (`src/jsonl_observer_system.h:209-257`).
- `jsonl_observer_tick_flat_refreshes(...)` already commits staged subtree on success and preserves last-good subtree on failure (`src/jsonl_observer_system.h:263-305`).
- Sketch safety loop policy is already codified in `jsonl_observer_tick_one(...)` with debounce/retry/auto-disable (`src/jsonl_observer_system.h:326-385`).
- `jsonl_observer_source_changed(...)` provides metadata+hash based change detection and is reusable for flat roots (`src/jsonl_sketch_import_job.h:117-153`).
- `JsonlObserverComp` already contains all required state fields (`observe_enabled`, `interval_ms`, `max_retries`, retry counters, source metadata, message history) (`src/components/jsonl_observer_comp.h:25-50`).
- Frame loop already runs both observer and flat refresh systems (`src/app.c:1902-1903`).
- Flat inspector currently has link/manual controls and status messaging; needs observe toggle + safety state surfacing (`src/ui/ui_entity_inspector.h:328-395`).

## Standard Stack

### Core
| Library/Module | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `JsonlObserverComp` | repo-local | Persist observer safety state | Already serialized and used across sketch/flat paths |
| `jsonl_observer_system.h` | repo-local | Observer policy + flat refresh orchestration | Centralized existing observer execution point |
| `jsonl_import_job.h` | repo-local | Background flat import/refresh job | Existing chunked non-sketch import engine |

### Supporting
| Library/Module | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `jsonl_sketch_import_job.h` (`jsonl_observer_source_changed`) | repo-local | Source-change detection helper | Flat auto loop change detection and debounce gating |
| `scene_serializer.h` | repo-local | Durable observer state | Ensure new flat auto state survives save/load |
| `ui_entity_inspector.h` | repo-local | Flat root controls | Expose observe toggle and safety status |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Reusing `jsonl_observer_source_changed` | New flat-only file check path | Duplicates fragile metadata/hash logic and risks policy drift |
| Auto tick on all `JsonlObserverComp` entities | Separate flat-only query filter | Must filter out sketches to avoid double-processing |

**Installation:** N/A (no new external dependencies).

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── jsonl_observer_system.h        # add flat auto tick policy
├── ui/ui_entity_inspector.h       # add Observe automatically control + warnings
├── tests/jsonl_flat_observer_manual_refresh_test.c
└── tests/jsonl_flat_observer_inspector_contract_test.c
```

### Pattern 1: Flat auto-loop policy mirror of sketch policy
**What:** Reuse same retry/debounce/auto-disable policy states already used in sketch observer tick.  
**When to use:** Any flat root with `JsonlObserverComp`, `linked=true`, `observe_enabled=true`, non-empty source path.  
**Example:**
```c
// Source: src/jsonl_observer_system.h (sketch precedent at lines 326-385)
if (obs->next_retry_at_ms != 0u && now_ms < obs->next_retry_at_ms) return false;
if (!jsonl_observer_source_changed(obs, obs->source_path, &source_changed)) { /* retry+disable */ }
if (source_changed && !jsonl_observer_is_flat_refresh_running(scene, root)) {
    (void)jsonl_observer_request_flat_refresh(scene, root, selection);
}
```

### Pattern 2: Keep execution transactional, schedule only
**What:** Auto loop should only schedule refresh attempts; actual subtree replacement stays in existing flat refresh slot machinery.  
**When to use:** All automatic attempts, including retries.  
**Anti-pattern to avoid:** Calling sketch reparse or ad-hoc subtree mutation in auto path.

### Anti-Patterns to Avoid
- **Bypassing flat refresh slots:** breaks one-in-flight guarantee and transaction semantics.
- **Consuming retry budget for empty/invalid path idle states:** violates D-04.
- **Auto-clearing source path when link OFF:** violates D-02.
- **Merging sketch and flat tick behavior without entity-type guards:** risks sketch coupling/perf regressions.

## Concrete Recommended Approach (Flat Auto Observer Loop)

1. Add `jsonl_observer_tick_one_flat(...)` in `jsonl_observer_system.h`.
2. Query `JsonlObserverComp` entities in `jsonl_observer_system_tick(...)`; for each entity:
   - if sketch exists: keep current `jsonl_observer_tick_one(...)`
   - else: run flat tick path.
3. Flat tick policy:
   - Early return if not linked or observe disabled.
   - If source path empty: push warning once per interval (or only on state transition), stay idle, no retry increment.
   - Enforce default `interval_ms`/`max_retries` if zero.
   - Respect `next_retry_at_ms` debounce.
   - Call `jsonl_observer_source_changed(...)`.
     - unchanged => reset retry_count; schedule next check at `now + interval`.
     - metadata/hash check failure => retry path; auto-disable at max_retries.
   - If changed and no in-flight slot => request flat refresh.
     - request success => set `next_retry_at_ms = now + interval`.
     - request fail with queue/running condition => coalesce (do not increment retry; reschedule at interval).
4. In `jsonl_observer_tick_flat_refreshes(...)`, on successful commit, stamp source state via `jsonl_observer_stamp_source_state(...)` to reduce false-positive reattempts.
5. Inspector (`ui_entity_inspector_draw_jsonl_flat_observer_controls`):
   - Add `Observe automatically` checkbox.
   - Link OFF should set `observe_enabled=false` immediately, keep `source_path`.
   - Link ON + valid source should default observe ON per D-01.
   - Surface retry counters / auto-disabled status text.
6. Keep `Re-import now` always available when source path exists (even if observe auto-disabled) per D-06.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| File change detection | New watcher/diff engine | `jsonl_observer_source_changed(...)` | Already supports metadata+hash semantics and shared behavior |
| Flat refresh transaction | Direct subtree mutation logic | Existing slot + commit/abort functions | Proven last-good preservation |
| Retry policy | New ad-hoc counters | Existing `JsonlObserverComp` fields | Persistence/UI/serializer already integrated |

**Key insight:** Phase 39 is a scheduling/policy extension, not a new import engine.

## Common Pitfalls

### Pitfall 1: Refresh thrash during write bursts
**What goes wrong:** Repeated refresh attempts while file is mid-write.  
**How to avoid:** Strict `next_retry_at_ms` gating + one-slot-per-root coalescing.

### Pitfall 2: Auto-disable on operator setup error
**What goes wrong:** Empty/missing path consumes retries and disables observe.  
**How to avoid:** Treat empty/invalid source as idle-warning state with no retry budget consumption (D-04).

### Pitfall 3: Breaking manual contract
**What goes wrong:** Auto-loop state blocks manual `Re-import now`.  
**How to avoid:** Keep manual trigger path independent of `observe_enabled`; only require path + no in-flight slot.

### Pitfall 4: Sketch overhead leakage
**What goes wrong:** Flat auto path calls sketch reparse pipeline.  
**How to avoid:** Restrict flat auto to `jsonl_observer_request_flat_refresh` and flat slot processing only.

## Risks/Failure Modes and Mitigation Options

| Risk | Failure Mode | Mitigation |
|------|--------------|------------|
| In-flight overlap | Auto tick repeatedly requests while refresh running | Coalesce by checking `jsonl_observer_is_flat_refresh_running` before request |
| Message spam | Warning every frame for invalid source | Emit warning on state change or interval boundary only |
| Retry drift | Flat behavior diverges from sketch safety defaults | Keep same retry increment/disable thresholds and message semantics |
| Persistence regression | New state not durable across save/load | Reuse existing serialized fields, add regression test coverage |

## Test Strategy (Phase 39)

### Update existing tests
- **`src/tests/jsonl_flat_observer_manual_refresh_test.c`**
  - Add auto-observe tests for:
    1) changed file triggers auto refresh attempt and commit
    2) rapid writes are debounced (no repeated in-flight starts)
    3) repeated failed auto attempts increment retry then disable observe
    4) manual refresh still works after auto-disable
- **`src/tests/jsonl_flat_observer_inspector_contract_test.c`**
  - Add required inspector strings/contract checks for:
    - `Observe automatically` control
    - safety status text (retry/auto-disabled)
    - retained existing controls (`Link file`, `Re-import now`, last result)

### Suggested new test file
- **`src/tests/jsonl_flat_observer_auto_safety_test.c`** (preferred for isolation)
  - unit-style policy tests using fixture rewrites + tick loop simulation.

## Code Examples

### Flat root filter in observer system tick
```c
// Source: src/jsonl_observer_system.h + src/app.c
if (ecs_world_get_sketch(scene->world, entity)) {
    (void)jsonl_observer_tick_one(scene, entity, now_ms);
} else {
    (void)jsonl_observer_tick_one_flat(scene, entity, now_ms, selection);
}
```

### Preserve manual + transactional baseline
```c
// Source: src/jsonl_observer_system.h lines 209-305
// Manual and auto both route through request + slot tick path.
(void)jsonl_observer_request_flat_refresh(scene, root_entity, selection);
jsonl_observer_tick_flat_refreshes(scene);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual-only flat refresh | Flat refresh engine + pending auto scheduler | Phase 38 complete; Phase 39 target | Need policy wiring, not engine rewrite |
| Sketch-only auto safety loop | Shared safety semantics across sketch + flat roots | Phase 39 | Consistent observer UX and safer write-window handling |

## Open Questions

1. **Should invalid path warnings be emitted once or at interval cadence?**
   - What we know: D-04 requires warning + no retry budget consumption.
   - Recommendation: emit once per state transition to avoid inspector spam.
2. **Where to store per-root coalesced “pending change while running” flag?**
   - What we know: one in-flight slot already enforced.
   - Recommendation: optional; start with interval polling + running gate (simpler, lower risk).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Build/test loop | ✓ | 4.3.2 | — |
| ctest | Validation Architecture commands | ✓ | 4.3.2 | Direct test binaries |
| ninja | Optional generator | ✗ | — | Visual Studio generator / existing build dirs |

**Missing dependencies with no fallback:** None.

**Missing dependencies with fallback:**
- `ninja` missing; use existing MSVC/VS generator build (`build-vulkan`) or current configured generator.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + native C test binaries (C11) |
| Config file | `CMakeLists.txt`, `src/CMakeLists.txt` |
| Quick run command | `ctest --output-on-failure -R "jsonl_flat_observer_(manual_refresh_test|inspector_contract_test)"` |
| Full suite command | `ctest --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| OBSF-04 | Auto refresh attempts on change + debounce + retry/disable safety | unit/integration | `ctest --output-on-failure -R "jsonl_flat_observer_(manual_refresh_test|inspector_contract_test|auto_safety_test)"` | ⚠️ `auto_safety_test` new (Wave 0) |

### Sampling Rate
- **Per task commit:** `ctest --output-on-failure -R "jsonl_flat_observer_(manual_refresh_test|inspector_contract_test)"`
- **Per wave merge:** `ctest --output-on-failure -R "jsonl_(observer_state_test|flat_observer_manual_refresh_test|flat_observer_inspector_contract_test)"`
- **Phase gate:** full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `src/tests/jsonl_flat_observer_auto_safety_test.c` — explicit OBSF-04 safety-loop coverage.
- [ ] `src/CMakeLists.txt` add target + `add_test(NAME jsonl_flat_observer_auto_safety_test ...)`.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/39-automatic-observer-safety-loop/39-CONTEXT.md` — locked decisions D-01..D-07.
- `src/jsonl_observer_system.h` — current sketch safety loop + flat manual transactional path.
- `src/jsonl_sketch_import_job.h` — source change detection helper and source-state stamping.
- `src/components/jsonl_observer_comp.h` — observer persisted state and defaults.
- `src/ui/ui_entity_inspector.h` — current flat inspector controls baseline.
- `src/tests/jsonl_flat_observer_manual_refresh_test.c` — transactional/manual contracts and last-good behavior tests.
- `src/tests/jsonl_flat_observer_inspector_contract_test.c` — current inspector contract checks.
- `src/app.c` — frame-loop integration points.
- `src/scene_serializer.h` — observer field durability.
- `src/CMakeLists.txt` — current CTest wiring and relevant test targets.
- `.planning/config.json` — `workflow.nyquist_validation=true`.

### Secondary (MEDIUM confidence)
- `AGENTS.md` — project conventions and C-first/header-only patterns.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - entirely repo-verified and already implemented in adjacent phase.
- Architecture: HIGH - direct code anchors for scheduler + transactional executor split.
- Pitfalls: HIGH - derived from existing retry semantics and flat transactional tests.

**Research date:** 2026-04-27  
**Valid until:** 2026-05-27

## RESEARCH COMPLETE

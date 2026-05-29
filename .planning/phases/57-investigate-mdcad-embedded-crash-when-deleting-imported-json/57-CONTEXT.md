# Phase 57: Investigate mdCAD embedded crash when deleting imported JSONL nodes and add crash diagnostics plus surfaced crash reasons across Avalonia and WPF sample hosts - Context

**Gathered:** 2026-05-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 57 covers a native mdCAD crash that appears when an embedded session starts with a JSONL model loaded from disk, the user selects an imported node in the mdCAD scene tree, and then deletes it with the Delete key. The user has reproduced this in the Avalonia and WPF minimal sample hosts and expects the same native crash to exist in the full Avalonia and WPF hosts because the failure comes from mdCAD itself rather than from the .NET shell.

This phase should both fix the crash and improve operator diagnostics around it: mdCAD should leave a useful crash/error log in the working directory, and the embedded sample hosts should surface the crash reason to the user instead of silently collapsing to a generic exit-code-only failure.

In scope:
- Reproduce and fix the native mdCAD crash path for deleting startup-imported JSONL content
- Cover the delete path reached from the scene hierarchy and downstream scene/entity cleanup
- Add native crash/error diagnostics written into the working directory used by the embedded runtime
- Surface crash reason details through the embedded Avalonia/WPF control backends and sample hosts
- Ensure both minimal and full sample hosts handle the unexpected mdCAD exit truthfully and consistently
- Add the right proof/tests/manual repro guidance to keep this crash path from regressing

Out of scope:
- Generic external telemetry, cloud crash reporting, or remote diagnostics services
- Broad redesign of the JSONL import system unrelated to this delete-crash path
- Non-embedded crash handling for every mdCAD runtime failure mode
- Replacing the existing Windows-only embedded-runtime boundary

</domain>

<decisions>
## Implementation Decisions

### User-facing scope
- **D-01:** The phase must treat the crash as a native mdCAD issue first, not as an Avalonia-vs-WPF shell mismatch.
- **D-02:** The fix must preserve the current embedded-host contract: mdCAD remains an external Windows process attached through the child-HWND seam.
- **D-03:** The operator outcome must improve in two places: a crash/error log written to the working directory, and a surfaced crash reason/message in the sample hosts.

### Reproduction and proof scope
- **D-04:** The primary repro is: embedded startup with a JSONL path already set, select an imported node in the mdCAD scene tree, press Delete, observe native process crash.
- **D-05:** Both full and minimal Avalonia/WPF hosts are part of proof because they share the same embedded backend/process-loss seam.
- **D-06:** Planning should explicitly cover whether the crash reproduces in both startup JSONL modes (plain startup import and startup import with live refresh enabled) if those paths differ materially.

### Diagnostics expectations
- **D-07:** Diagnostics should build on the existing host capture of child stdout/stderr and unexpected-session-loss messaging instead of inventing a separate host-to-runtime IPC channel.
- **D-08:** The native side should emit a stable crash/error record in the working directory that the host can point to or quote when the child process dies unexpectedly.
- **D-09:** Host messaging must stay truthful: if the native runtime crashes, the host should report that it observed an unexpected mdCAD exit and include the best available native reason/log reference it actually has.

### The agent's discretion
- Choose the exact log filename/format and whether it is crash-only or broader runtime-error logging, as long as it lands in the working directory and is practical for operators.
- Choose the exact split between native mdCAD hardening and .NET host-surface updates, as long as both sides are planned together.
- Choose the right proof mix (native/unit/integration/manual) as long as the repro path and surfaced-diagnostics behavior are credibly locked down.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 57 roadmap entry
- `.planning/REQUIREMENTS.md` — current host/runtime requirement baseline
- `.planning/PROJECT.md` — project truth and Windows embedding boundary
- `.planning/STATE.md` — current milestone state and verification handoff context

### Phase 56 host/control baseline
- `.planning/phases/56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-/56-CONTEXT.md`
- `.planning/phases/56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-/56-RESEARCH.md`

### Native crash-path anchors
- `src/startup_jsonl_import_controller.h` — startup JSONL import state machine used when the host launches with `JsonlPath`
- `src/ui/ui_scene_hierarchy.h` — scene-tree Delete menu path funnels into `ui_scene_hierarchy_delete_entities`
- `src/ecs/ecs_scene.h` — `scene_remove_entity` recursive removal path and cleanup ordering
- `src/ecs/ecs_world.h` — low-level entity deletion and geometry/pick cleanup
- `src/jsonl_observer_system.h` — JSONL-linked flat-refresh cleanup/selection interaction helpers
- `src/platform/win32_embed.h` — current embedded-startup stderr failure surface

### Embedded host/control anchors
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs`
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs`
- `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs`
- `samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs`
- `samples/avalonia-host/`
- `samples/avalonia-host-minimal/`
- `samples/wpf-host/`
- `samples/wpf-host-minimal/`

</canonical_refs>

<code_context>
## Existing Code Insights

### Native delete path currently in play
- `ui_scene_hierarchy_delete_entities` cancels a running flat refresh for the selected root/anchor, removes the entity from the selection buffer, then calls `scene_remove_entity`.
- `scene_remove_entity` recursively deletes children, unlinks geometry constraints, unlinks constraint participants, frees instance-buffer slots, and finally calls `ecs_world_delete_entity`.
- `ecs_world_delete_entity` frees pick IDs and geometry allocations before the ECS entity is deleted.

### Startup JSONL path in play
- `startup_jsonl_import_controller_tick` arms and starts a `jsonl_import_job`, updates status/error text, and can configure observer/live-refresh linkage with `jsonl_import_job_set_observer_contract`.
- The user’s current reproduction is specifically for models loaded from disk on startup, not only for interactively imported files later in the session.

### Current embedded diagnostics surface
- The native embedded startup path currently only has a hard stderr failure helper in `mdcad_win32_embed_fail_startup(...)`, which prints a single line and exits.
- Both Avalonia and WPF Windows embed backends already capture child stdout/stderr, keep one `_capturedFailureLine`, and surface unexpected process loss through `UnexpectedSessionLoss`.
- Today that host-side capture is strongest for startup-failure lines containing `Embedded startup failed:`; there is no broader working-directory crash log contract for post-attach native crashes.

### Current sample-host startup behavior
- The minimal Avalonia and WPF hosts bind `JsonlPath` directly into `MdCadEmbeddedControl` and auto-start sealed mode with no extra crash-diagnostics UI.
- The full Avalonia and WPF hosts already maintain runtime/harness status text, making them the natural place to surface richer crash/log information.

</code_context>

<specifics>
## Specific Ideas

- The plan should explicitly investigate whether deleting an imported child/root leaves stale selection, observer, GPU-slot, or hierarchy references that later code touches after free/delete.
- The plan should call out native repro automation where possible, but it should also include a manual embedded-host repro script because the user described a UI-driven crash path.
- The host-side work should likely extend the existing stderr/unexpected-exit capture rather than building a new communications channel.
- Since the user expects the crash to reproduce across all hosts, the fix/proof split should probably isolate native mdCAD crash prevention first, then finish with shared embedded-host diagnostics surfacing.

</specifics>

<deferred>
## Deferred Ideas

- Full native minidump infrastructure or symbolized stack-trace pipelines
- Generic crash-report uploader/telemetry
- Broader redesign of scene-hierarchy delete semantics beyond what the imported-JSONL crash requires
- Cross-platform crash handling promises outside the Windows embedded runtime

</deferred>

---

*Phase: 57-investigate-mdcad-embedded-crash-when-deleting-imported-json*
*Context gathered: 2026-05-28*

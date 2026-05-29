# Phase 57: Investigate mdCAD embedded crash when deleting imported JSONL nodes and add crash diagnostics plus surfaced crash reasons across Avalonia and WPF sample hosts - Research

**Researched:** 2026-05-29  
**Domain:** Native mdCAD delete-path hardening plus embedded-host crash diagnostics  
**Confidence:** MEDIUM

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
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

### The agent's Discretion
- Choose the exact log filename/format and whether it is crash-only or broader runtime-error logging, as long as it lands in the working directory and is practical for operators.
- Choose the exact split between native mdCAD hardening and .NET host-surface updates, as long as both sides are planned together.
- Choose the right proof mix (native/unit/integration/manual) as long as the repro path and surfaced-diagnostics behavior are credibly locked down.

### Deferred Ideas (OUT OF SCOPE)
- Full native minidump infrastructure or symbolized stack-trace pipelines
- Generic crash-report uploader/telemetry
- Broader redesign of scene-hierarchy delete semantics beyond what the imported-JSONL crash requires
- Cross-platform crash handling promises outside the Windows embedded runtime
</user_constraints>

## Summary

The delete crash is most likely in the native delete chain that starts in `ui_scene_hierarchy_delete_entities(...)`, snapshots the subtree for undo, then recursively frees scene/render resources. That path is common to embedded and non-embedded mdCAD, which matches the user’s framing that this is a native mdCAD defect rather than an Avalonia/WPF mismatch. The three highest-probability root-cause zones are: (1) bulk-delete undo snapshotting of imported subtrees, (2) recursive scene/resource teardown for imported renderables, and (3) startup-import-specific observer metadata/cleanup asymmetry. [CITED: src/ui/ui_scene_hierarchy.h] [CITED: src/undo_redo_exec.h] [CITED: src/ecs/ecs_scene.h] [CITED: src/ecs/ecs_world.h] [CITED: src/startup_jsonl_import_controller.h] [CITED: src/jsonl_import_job.h]

The best diagnostics architecture is to keep one source of truth on the native side and reuse the existing host seams on the .NET side. The shared Windows start-info path already launches mdCAD with `WorkingDirectory == runtime.RuntimeRoot`, and both Windows backends already capture stdout/stderr plus raise `UnexpectedSessionLoss`. That makes the practical design: write a deterministic native crash/error record into the working directory, emit one stable stderr summary line that references that record, then have the existing backends include that summary/log reference in the unexpected-exit detail they already surface to the controls. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: src/platform/win32_embed.h]

The current proof gap is real: the repo already has native delete coverage for linked flat-refresh roots, but not for the exact startup-import + scene-hierarchy + embedded-diagnostics path the user reported. Phase 57 should therefore add one native regression lane for startup-import deletes and one host/control diagnostics lane for post-attach unexpected-exit reporting, then finish with a manual repro checklist across the four sample hosts. [CITED: src/tests/jsonl_flat_observer_manual_refresh_test.c] [CITED: src/tests/startup_jsonl_import_controller_test.c] [CITED: samples/avalonia-host/MainWindow.axaml.cs] [CITED: samples/wpf-host/MainWindow.xaml.cs]

**Primary recommendation:** Fix and instrument the native delete path first, then extend the existing stderr/`UnexpectedSessionLoss` seam so both controls and the full sample hosts can surface a working-directory crash reason without adding new IPC. [CITED: src/ui/ui_scene_hierarchy.h] [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Imported-node delete crash prevention | Native mdCAD process | — | The delete path is entirely native: scene hierarchy → undo snapshot → scene teardown → ECS deletion. [CITED: src/ui/ui_scene_hierarchy.h] [CITED: src/undo_redo_exec.h] [CITED: src/ecs/ecs_scene.h] [CITED: src/ecs/ecs_world.h] |
| Crash/error record creation in working directory | Native mdCAD process | Windows embed helper | The child process owns the fault context; `win32_embed.h` is the existing embedded-specific stderr seam. [CITED: src/platform/win32_embed.h] |
| Working-directory discovery | Shared Windows start-info builder/backend | Native mdCAD process | The host already chooses the runtime working directory by process start-info. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] [CITED: samples/wpf-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] |
| Unexpected-exit reason aggregation | Avalonia/WPF Windows backends | Control shells | Both backends already capture child stdout/stderr and raise `UnexpectedSessionLoss`. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] |
| Operator-visible crash reason in minimal hosts | Control warning surface | Backend unexpected-exit detail | Minimal hosts currently provide almost no host chrome, so the control must remain the primary user-facing surface there. [CITED: samples/avalonia-host-minimal/MainWindow.axaml] [CITED: samples/wpf-host-minimal/MainWindow.xaml] [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml] [CITED: samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml] |
| Operator-visible crash reason in full hosts | Sample host status lines | Control warning/detail surface | Full hosts already own `RuntimeStatusTextBlock` / `HarnessStatusTextBlock`, so they are the right extra mirror for backend-provided crash detail. [CITED: samples/avalonia-host/MainWindow.axaml] [CITED: samples/avalonia-host/MainWindow.axaml.cs] [CITED: samples/wpf-host/MainWindow.xaml] [CITED: samples/wpf-host/MainWindow.xaml.cs] |

## Standard Stack

### Core
| Library / Component | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `ui_scene_hierarchy_delete_entities(...)` + `undo_cmd_bulk_delete_entities(...)` + `scene_remove_entity(...)` | repo current | Canonical imported-node delete path | This is the exact production path reached from the hierarchy UI; fixing elsewhere would miss the user repro. [CITED: src/ui/ui_scene_hierarchy.h] [CITED: src/undo_redo_exec.h] [CITED: src/ecs/ecs_scene.h] |
| Existing embedded Windows backends (`WindowsMdCadEmbedBackend`) | repo current | Child-process stderr capture, attach detection, unexpected-exit surfacing | Both UI stacks already use the same process-loss seam; Phase 57 should extend it instead of bypassing it. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] |
| Native embedded startup/error seam in `win32_embed.h` | repo current | Current native stderr failure surface | This is the natural place to centralize embedded-mode crash/error emission. [CITED: src/platform/win32_embed.h] |

### Supporting
| Library / Component | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Startup JSONL import controller + import job observer contract | repo current | Reproduce and lock startup-import-specific state | Use for proving both plain startup import and startup live-refresh startup mode. [CITED: src/startup_jsonl_import_controller.h] [CITED: src/jsonl_import_job.h] |
| JSONL observer flat-refresh helpers | repo current | Cleanup/cancel behavior when imported roots have observer metadata or active refresh state | Use when the selected entity is the root or lives under a refreshing linked root. [CITED: src/jsonl_observer_system.h] |
| Control warning/detail surfaces | repo current | Show crash detail without adding host IPC | Use as the minimal-host operator surface and fallback surface in full hosts. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs] [CITED: samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Existing stderr + `UnexpectedSessionLoss` seam | New host/runtime IPC channel | Rejected: contradicts D-07 and duplicates already-working host capture paths. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md] |
| Single working-directory crash file | Full minidump/symbol pipeline | Deferred by scope; too large for this phase. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md] |
| Control warning surface in minimal hosts | New minimal-host diagnostic chrome | Possible, but higher churn than necessary because minimal hosts currently are just one window plus control. [CITED: samples/avalonia-host-minimal/MainWindow.axaml.cs] [CITED: samples/wpf-host-minimal/MainWindow.xaml.cs] |

**Installation:** No new external packages are recommended for this phase. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md]

## Architecture Patterns

### System Architecture Diagram

```text
Delete key / scene-tree delete
        |
        v
ui_scene_hierarchy_delete_entities(...)
        |
        +--> undo_cmd_bulk_delete_entities(...)  -- snapshot imported subtree before free
        |
        +--> jsonl_observer_cancel_flat_refresh_for_root(...) when applicable
        |
        +--> scene_remove_entity(...)
                 |
                 +--> recursive child delete
                 +--> scene_free_entity_slots(...)
                 +--> ecs_world_delete_entity(...)
                          |
                          +--> free pick id
                          +--> free GeometryComp allocations
                          +--> ecs_delete(...)
        |
        v
native crash? ------------------------------+
        |                                   |
        v                                   |
write working-dir crash/error record        |
emit stderr summary line -------------------+
        |
        v
Avalonia/WPF backend stdout/stderr capture
        |
        v
UnexpectedSessionLoss(detail with log reference)
        |
        +--> control warning/detail surface
        +--> full-host harness status line mirror
```

### Recommended Project Structure
```text
src/
├── ui/                     # scene-hierarchy delete entry point and UI repro anchor
├── ecs/                    # recursive scene/entity teardown and resource cleanup
├── platform/               # embedded-only stderr/crash log seam
└── tests/                  # native startup-import delete regression coverage

samples/
├── avalonia-mdcad-control/ # existing backend/control warning seam
├── wpf-mdcad-control/      # existing backend/control warning seam
├── avalonia-host/          # full host harness status mirror
├── wpf-host/               # full host harness status mirror
├── avalonia-host-minimal/  # sealed minimal proof surface
└── wpf-host-minimal/       # sealed minimal proof surface
```

### Pattern 1: Fix the real delete path before touching host UX
**What:** Reproduce and instrument the production delete chain that starts in `ui_scene_hierarchy_delete_entities(...)`, not a synthetic cleanup helper. [CITED: src/ui/ui_scene_hierarchy.h]  
**When to use:** Always for this phase’s crash fix. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md]  
**Example:**
```c
// Source: src/ui/ui_scene_hierarchy.h
static inline void ui_scene_hierarchy_delete_entities(ui_scene_hierarchy_state_t *state,
                                                      const ecs_entity_t *entities,
                                                      int count) {
    if (state->undo_redo) {
        undo_cmd_bulk_delete_entities(state->undo_redo, (ecs_entity_t*)entities, count);
    }
    for (int i = 0; i < count; i++) {
        ecs_entity_t e = entities[i];
        ecs_entity_t refresh_anchor = ui_scene_hierarchy_find_refreshing_flat_anchor(state, e);
        jsonl_observer_cancel_flat_refresh_for_root(state->scene, refresh_anchor != 0 ? refresh_anchor : e);
        if (state->selection) selection_remove(state->selection, e);
        scene_remove_entity(state->scene, e);
    }
    if (state->selection) selection_prune_dead(state->selection);
}
```

### Pattern 2: Reuse the existing unexpected-exit seam
**What:** Extend backend stderr capture and `UnexpectedSessionLoss` composition instead of adding a new channel. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs]  
**When to use:** For both startup-failure and post-attach crash reporting. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md]  
**Example:**
```csharp
// Source: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs
private void CleanupAfterUnexpectedSessionLoss(string detail)
{
    _attachTimer.Stop();
    _resizeSyncTimer.Stop();
    DisposeCurrentProcess();
    _attachedChildHwnd = IntPtr.Zero;
    _launchParentHwnd = IntPtr.Zero;
    _capturedFailureLine = null;
    _setLaunchStatus("timeout/failure");
    _setAttachStatus("idle");
    _updatePreLaunchStatus(_captureLaunchSnapshot());
    _updateControlState();
    UnexpectedSessionLoss?.Invoke(detail);
}
```

### Pattern 3: Make minimal hosts consume control-owned warning text, not new host orchestration
**What:** Keep minimal hosts minimal; rely on the control’s existing warning surface for crash detail. [CITED: samples/avalonia-host-minimal/MainWindow.axaml] [CITED: samples/wpf-host-minimal/MainWindow.xaml] [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml] [CITED: samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml]  
**When to use:** For Avalonia/WPF minimal sample coverage in this phase. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md]  

### Anti-Patterns to Avoid
- **New diagnostics IPC channel:** The host already redirects stdout/stderr and already raises `UnexpectedSessionLoss`; adding another protocol widens scope for no Phase 57 value. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs]
- **Investigating only `scene_remove_entity(...)`:** Delete always snapshots undo first, so root-cause analysis that ignores `undo_cmd_bulk_delete_entities(...)` can miss the actual faulting step. [CITED: src/ui/ui_scene_hierarchy.h] [CITED: src/undo_redo_exec.h]
- **Assuming “no live refresh” means “no observer state”:** Startup import always captures observer contract and source path, even when link is disabled. [CITED: src/startup_jsonl_import_controller.h] [CITED: src/jsonl_import_job.h] [CITED: src/tests/startup_jsonl_import_controller_test.c]

## Root-Cause Investigation Priorities

### Priority 1: Bulk-delete undo snapshot on imported subtrees
**Why it is high probability:** The hierarchy delete path records an undo snapshot for the full subtree before actual deletion. That makes `undo_cmd_bulk_delete_entities(...)` the first imported-data-heavy operation on Delete, and it deep-copies geometry data manually. [CITED: src/ui/ui_scene_hierarchy.h] [CITED: src/undo_redo_exec.h]  
**Evidence:**
- `ui_scene_hierarchy_delete_entities(...)` always calls `undo_cmd_bulk_delete_entities(...)` before `scene_remove_entity(...)`. [CITED: src/ui/ui_scene_hierarchy.h]
- `undo_cmd_bulk_delete_entities(...)` expands the subtree recursively and snapshots every entity. [CITED: src/undo_redo_exec.h]
- `undo_snapshot_entity(...)` performs raw geometry copies for polylines, polygons, meshes, labels, sketch state, constraints, and endpoints. [CITED: src/undo_redo_exec.h]
- Existing startup-import tests stop at import completion/error and do not exercise delete/undo on startup-imported content. [CITED: src/tests/startup_jsonl_import_controller_test.c]

### Priority 2: Recursive scene/resource teardown of imported renderables
**Why it is high probability:** The actual destructive path frees instance-buffer slots, then geometry allocations, then deletes the ECS entity. Imported JSONL content is exactly the content most likely to exercise multi-slot renderables and heap-backed geometry. [CITED: src/ecs/ecs_scene.h] [CITED: src/ecs/ecs_world.h]  
**Evidence:**
- `scene_remove_entity(...)` recursively deletes children, unlinks constraints, frees slots, then calls `ecs_world_delete_entity(...)`. [CITED: src/ecs/ecs_scene.h]
- `scene_free_entity_slots(...)` frees different slot shapes for lines, points, point clouds, meshes, polylines, arcs, polygons, helixes, and beziers. [CITED: src/ecs/ecs_scene.h]
- `ecs_world_delete_entity(...)` separately frees pick IDs and `GeometryComp` dynamic allocations before `ecs_delete(...)`. [CITED: src/ecs/ecs_world.h]
- Existing native coverage proves delete-after-refresh for linked flat roots, but not startup-imported child/subtree deletes through the hierarchy path. [CITED: src/tests/jsonl_flat_observer_manual_refresh_test.c]

### Priority 3: Startup-import observer metadata and refresh-cancel asymmetry
**Why it is medium probability:** Startup import attaches observer metadata even when live refresh is disabled, and the hierarchy delete path conditionally cancels flat refreshes. That makes startup-imported roots structurally different from ordinary hand-created scene entities. [CITED: src/startup_jsonl_import_controller.h] [CITED: src/jsonl_import_job.h] [CITED: src/jsonl_observer_system.h]  
**Evidence:**
- `startup_jsonl_import_controller_tick(...)` always calls `jsonl_import_job_set_observer_contract(...)`. [CITED: src/startup_jsonl_import_controller.h]
- Startup-import tests assert that observer-contract capture happens both with and without live refresh, with only `link_enabled` changing. [CITED: src/tests/startup_jsonl_import_controller_test.c]
- `jsonl_observer_cancel_flat_refresh_for_root(...)` only resets active slots when one exists; deleting an unlinked startup-import root is a different state from deleting a linked-refresh root mid-refresh. [CITED: src/jsonl_observer_system.h]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Crash reason transport to hosts | New socket/pipe/custom IPC channel | Existing stderr capture + `UnexpectedSessionLoss` detail composition | Already implemented in both Windows backends; phase scope explicitly prefers this seam. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md] |
| Alternate delete logic for imported nodes | Special “imported delete” code path | Harden/instrument `ui_scene_hierarchy_delete_entities(...)` → undo → scene removal | The user repro hits the normal delete path; a side path would not protect the real entry point. [CITED: src/ui/ui_scene_hierarchy.h] |
| Host-specific crash log locations | Separate Avalonia/WPF path logic | The existing runtime working directory from start-info | One location keeps the native and host sides consistent. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] [CITED: samples/wpf-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] |

**Key insight:** The phase already has the right seams; it needs more truthful data flowing through them, not new architecture. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md]

## Common Pitfalls

### Pitfall 1: Treating the crash as a host-framework bug
**What goes wrong:** Investigation starts in Avalonia/WPF backend code and misses the actual fault.  
**Why it happens:** The crash is seen only when embedded, so the shell feels suspicious.  
**How to avoid:** Start from the native delete chain; only use hosts to prove surfaced diagnostics. [CITED: src/ui/ui_scene_hierarchy.h] [CITED: src/ecs/ecs_scene.h] [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs]  
**Warning signs:** Repro/fix work changes host code without adding any native delete regression. [CITED: src/tests/startup_jsonl_import_controller_test.c]

### Pitfall 2: Capturing only startup stderr and losing post-attach crash detail
**What goes wrong:** The backend reports only `exit code N` after attach.  
**Why it happens:** `_capturedFailureLine` is currently populated only for lines containing `Embedded startup failed:`. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs]  
**How to avoid:** Capture the last relevant stderr/stdout fatal line regardless of startup phase and pair it with the working-directory crash log path. [CITED: src/platform/win32_embed.h] [ASSUMED]  
**Warning signs:** Full hosts show `mdCAD exited after attach (exit code ...)` with no useful reason. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs]

### Pitfall 3: Forgetting that minimal hosts have no host-owned status line
**What goes wrong:** Crash detail is added only to full-host harness text, leaving minimal samples silent.  
**Why it happens:** Minimal hosts are just one control with a bound `JsonlPath`. [CITED: samples/avalonia-host-minimal/MainWindow.axaml.cs] [CITED: samples/wpf-host-minimal/MainWindow.xaml.cs]  
**How to avoid:** Ensure the control warning surface itself shows the best available crash reason/log reference in sealed mode. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml] [CITED: samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml]  
**Warning signs:** Full hosts report the crash, but minimal hosts still appear to “just die” or go blank. [CITED: samples/avalonia-host/MainWindow.axaml.cs] [CITED: samples/wpf-host/MainWindow.xaml.cs]

## Code Examples

Verified patterns from current repo code:

### Native delete chain to instrument
```c
// Source: src/ecs/ecs_scene.h
static inline void scene_remove_entity(ecs_scene_t *scene, ecs_entity_t e) {
    if (!ecs_is_alive(scene->world->world, e)) return;
    ecs_entity_t parent = ecs_world_get_parent(scene->world, e);
    if (ecs_world_is_selected(scene->world, e)) ecs_world_deselect(scene->world, e);
    if (ecs_world_is_hovered(scene->world, e)) ecs_world_clear_hovered(scene->world, e);
    ecs_entity_t children[64];
    int child_count;
    while ((child_count = ecs_world_get_children(scene->world, e, children, 64)) > 0) {
        for (int i = 0; i < child_count; i++) {
            scene_remove_entity(scene, children[i]);
        }
    }
    if (ecs_world_get_geometry(scene->world, e)) scene_geometry_unlink_constraints(scene, e);
    ConstraintComp *constraint = ecs_world_get_constraint(scene->world, e);
    if (constraint) scene_constraint_unlink_participants(scene, constraint, e);
    scene_free_entity_slots(scene, e);
    ecs_world_delete_entity(scene->world, e);
    if (parent != 0 && ecs_is_alive(scene->world->world, parent) && scene_is_sketch(scene, parent)) {
        scene_refresh_sketch_metadata(scene, parent);
    }
}
```

### Existing host unexpected-exit seam to extend
```csharp
// Source: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs
private void OnResizeSyncTick(object? sender, EventArgs e)
{
    if (_attachedChildHwnd == IntPtr.Zero)
    {
        _resizeSyncTimer.Stop();
        return;
    }

    if (_mdcadProcess?.HasExited == true)
    {
        CleanupAfterUnexpectedSessionLoss($"mdCAD exited after attach (exit code {_mdcadProcess.ExitCode}).");
        return;
    }

    if (!EnsureAttachedLifecycleIsHealthy())
    {
        return;
    }

    SyncAttachedChildBounds();
}
```

### Existing control warning/detail surface
```csharp
// Source: samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs
private void OnBackendUnexpectedSessionLoss(string detail)
{
    SetLaunchWarning(detail);
    UpdateControlState();
    _ = RunCoordinatorTaskAsync(_sessionCoordinator.StopAsync(), rethrow: false);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Startup failure only: one stderr line via `mdcad_win32_embed_fail_startup(...)` | Phase 57 should preserve that seam but broaden it to post-attach crash/error records plus stderr summaries | Current repo state before Phase 57 | Operators get actionable crash reason after attach instead of only an exit code. [CITED: src/platform/win32_embed.h] [ASSUMED] |
| Host backends only latch stderr lines containing `Embedded startup failed:` | Phase 57 should treat crash/fatal stderr lines generically and pair them with the working-dir log | Current repo state before Phase 57 | Enables truthful post-attach crash reporting. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [ASSUMED] |
| Native delete tests focus on linked flat-refresh roots | Phase 57 needs startup-import delete regression coverage and host diagnostics regression coverage | Current repo state before Phase 57 | Locks the exact user repro instead of adjacent cases only. [CITED: src/tests/jsonl_flat_observer_manual_refresh_test.c] [CITED: src/tests/startup_jsonl_import_controller_test.c] |

**Deprecated/outdated:**
- “Exit code only” host messaging for post-attach crashes is no longer sufficient for this phase. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md]

## Open Questions (RESOLVED)

1. **Does the crash reproduce when deleting a startup-imported child, root, or both?**
   - **Selected answer:** Plan and proof should cover both the imported root anchor and at least one imported geometry child beneath it.
   - **Why:** The user repro only says “an imported node,” and the native delete path differs materially between observer-root cleanup and plain subtree delete. Covering both prevents the phase from overfitting to a single tree level. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md] [CITED: src/ui/ui_scene_hierarchy.h]

2. **Does the crash require startup live refresh to be enabled?**
   - **Selected answer:** No assumption of refresh-only scope is allowed; proof must cover both plain startup import and startup import with live refresh enabled wherever that mode exists.
   - **Why:** Startup import captures observer/source metadata in both modes, while linked mode adds refresh-specific behavior. The phase should therefore lock both startup modes rather than guessing which one is responsible. [CITED: src/tests/startup_jsonl_import_controller_test.c] [CITED: src/jsonl_import_job.h]

3. **Should full hosts mirror crash detail outside the control, or is the control warning enough?**
   - **Selected answer:** Yes, full hosts should mirror the same observed crash/log detail, and they should do it through a small public control relay seam that keeps the control warning text as the source of truth.
   - **Why:** Full hosts already own harness/runtime status surfaces, while minimal hosts rely almost entirely on the control UI. That makes a control-owned warning plus full-host mirror the lowest-churn way to satisfy D-05 and D-09 across both host tiers. [CITED: samples/avalonia-host/MainWindow.axaml.cs] [CITED: samples/wpf-host/MainWindow.xaml.cs] [CITED: samples/avalonia-host-minimal/MainWindow.axaml.cs] [CITED: samples/wpf-host-minimal/MainWindow.xaml.cs]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | A working-directory crash handler should use Windows top-level exception handling plus fatal-signal/fatal-error hooks so unexpected native termination still produces a record. [ASSUMED] | Summary / State of the Art / Security Domain | Medium — implementation may need a different native hook strategy on this codebase. |
| A2 | Full hosts should consume a small public control relay seam for `UnexpectedSessionLoss` detail instead of scraping internal warning state. | Open Questions / Testing | Low — if the exact seam changes, the planning intent still stands: full hosts need automated proof of mirrored detail. |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| `dotnet` | Avalonia/WPF control and host diagnostics tests | ✓ | 10.0.300 | — |
| `cmake` | Native test/build lane | ✓ | 4.3.2 | — |
| `ctest` | Native regression execution | ✓ | 4.3.2 | — |
| `git` | Repo workflows | ✓ | 2.51.1.windows.1 | — |

**Missing dependencies with no fallback:**
- None. [VERIFIED: local tool probe]

**Missing dependencies with fallback:**
- None. [VERIFIED: local tool probe]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Native C tests via CTest + .NET xUnit test projects for Avalonia/WPF control layers [VERIFIED: local tool probe] |
| Config file | Native: `build/` CTest config; .NET: project-local xUnit test projects [CITED: build/ctest -N output] [CITED: samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj] [CITED: samples/wpf-mdcad-control.tests/MdCad.Wpf.Control.Tests.csproj] |
| Quick run command | `ctest --test-dir build -R "jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test"` plus targeted `dotnet test` filter(s) below [VERIFIED: local tool probe] |
| Full suite command | `ctest --test-dir build` + `dotnet test samples\\avalonia-mdcad-control.tests\\MdCad.Avalonia.Control.Tests.csproj` + `dotnet test samples\\wpf-mdcad-control.tests\\MdCad.Wpf.Control.Tests.csproj` [VERIFIED: local tool probe] |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| P57-01 [ASSUMED] | Startup-imported node delete no longer crashes native mdCAD | native regression | `ctest --test-dir build -R startup_jsonl_delete` [ASSUMED] | ❌ Wave 0 |
| P57-02 [ASSUMED] | Startup-import plain mode vs live-refresh mode both covered for delete path | native regression | `ctest --test-dir build -R startup_jsonl_delete` [ASSUMED] | ❌ Wave 0 |
| P57-03 [ASSUMED] | Native embedded failures write a working-dir crash/error record and emit a truthful fatal summary line for the host to consume | native regression | `ctest --test-dir build -R win32_embed_diagnostics_test` [ASSUMED] | ❌ Wave 0 |
| P57-04 [ASSUMED] | Unexpected embedded exit surfaces the working-dir crash reason truthfully through both Avalonia and WPF backends/controls/full hosts | xUnit + host tests | Targeted Avalonia/WPF control tests plus targeted full-host harness tests [ASSUMED] | ❌ Wave 0 |
| P57-05 [ASSUMED] | Full/minimal sample hosts prove the delete repro no longer crashes and that surfaced diagnostics remain control-owned/minimal and mirrored/full where intended | manual + targeted host/control tests | Host-control tests + manual checklist | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build -R "jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test"` and the touched control test project’s filtered `dotnet test` lane. [VERIFIED: local tool probe]
- **Per wave merge:** Full native + Avalonia + WPF test commands above. [VERIFIED: local tool probe]
- **Phase gate:** Full suite green plus manual repro/diagnostics checklist across Avalonia full/minimal and WPF full/minimal hosts. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md]

### Wave 0 Gaps
- [ ] `src/tests/startup_jsonl_delete_regression_test.c` [ASSUMED] — reproduces startup-import delete on root and child, with and without startup live refresh.
- [ ] Extend `src/tests/jsonl_flat_observer_manual_refresh_test.c` or sibling native test to assert slot/observer cleanliness for startup-import delete, not just manual-refresh delete. [CITED: src/tests/jsonl_flat_observer_manual_refresh_test.c]
- [ ] Add Avalonia control/backend tests that simulate post-attach unexpected exit with captured stderr/log-path detail and assert warning/detail text updates. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs]
- [ ] Add WPF control/backend tests that simulate post-attach unexpected exit with captured stderr/log-path detail and assert warning/detail text updates. [CITED: samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs]
- [ ] Add a manual checklist row for all four hosts covering working-directory log creation plus surfaced crash reason text. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | yes | Fixed log filename/path rooted under the runtime working directory; do not let child stderr choose arbitrary paths. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] [ASSUMED] |
| V6 Cryptography | no | — |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Log spoofing via stale crash file | Repudiation | Clear or rewrite the crash record on clean startup so hosts do not surface an old crash as the current one. [ASSUMED] |
| Path confusion for diagnostic record | Tampering | Keep the native log filename fixed and anchored to the working directory already chosen by the host start-info. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] [ASSUMED] |
| Over-reporting unverified crash causes in host UI | Repudiation | Host text must say it observed an unexpected exit and include only captured stderr/log data it actually has. [CITED: .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-CONTEXT.md] |

## Sources

### Primary (HIGH confidence)
- `src/ui/ui_scene_hierarchy.h` - hierarchy delete entry point, refresh-cancel logic, selection pruning, cache invalidation.
- `src/undo_redo_exec.h` - bulk delete snapshot ordering and snapshot deep-copy behavior.
- `src/ecs/ecs_scene.h` - recursive entity deletion and slot cleanup.
- `src/ecs/ecs_world.h` - low-level entity deletion and geometry free path.
- `src/startup_jsonl_import_controller.h` - startup import state machine and observer-contract setup.
- `src/jsonl_import_job.h` - observer metadata persistence on imported roots.
- `src/jsonl_observer_system.h` - flat-refresh cancel/reset/commit behavior.
- `src/platform/win32_embed.h` - current embedded stderr failure path.
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - stdout/stderr capture and unexpected session-loss behavior.
- `samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - stdout/stderr capture and unexpected session-loss behavior.
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml(.cs)` - control warning/detail surface.
- `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml(.cs)` - control warning/detail surface.
- `samples/avalonia-host/*`, `samples/wpf-host/*`, `samples/avalonia-host-minimal/*`, `samples/wpf-host-minimal/*` - host-visible status surfaces and minimal-host limitations.
- `src/tests/jsonl_flat_observer_manual_refresh_test.c` - existing delete-after-refresh/delete-during-refresh coverage.
- `src/tests/startup_jsonl_import_controller_test.c` - startup import mode coverage.
- Local tool probes (`ctest -N`, `dotnet test --list-tests`, tool versions) - current validation and environment availability. [VERIFIED: local tool probe]

### Secondary (MEDIUM confidence)
- Microsoft Learn: `SetUnhandledExceptionFilter` - process-level top-level exception handler capability for Windows crash recording. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setunhandledexceptionfilter]

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - The phase should reuse existing repo seams rather than adopt new libraries, and those seams were inspected directly in code/tests. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs]
- Architecture: HIGH - Working-directory ownership, stderr capture, control warning surfaces, and host status surfaces are all present and directly inspectable. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] [CITED: samples/wpf-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] [CITED: samples/avalonia-host/MainWindow.axaml.cs] [CITED: samples/wpf-host/MainWindow.xaml.cs]
- Root-cause ranking: MEDIUM - The likely fault zones are clear from the production call chain, but the exact crashing step still needs runtime repro or added instrumentation. [CITED: src/ui/ui_scene_hierarchy.h] [CITED: src/undo_redo_exec.h] [CITED: src/ecs/ecs_scene.h]
- Pitfalls: HIGH - The main pitfalls are direct consequences of current code structure and current proof gaps. [CITED: src/tests/startup_jsonl_import_controller_test.c] [CITED: src/tests/jsonl_flat_observer_manual_refresh_test.c]

**Research date:** 2026-05-29  
**Valid until:** 2026-06-28

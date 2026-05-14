# Project Research Summary

**Project:** mdCAD v1.8 — Embeddable Windows JSONL Viewer  
**Domain:** Windows child-HWND embedding, CLI startup import, and hosted large-JSONL viewing  
**Researched:** 2026-05-14  
**Confidence:** MEDIUM

## Executive Summary

mdCAD v1.8 is best treated as a **Windows embedding and startup-orchestration milestone**, not a renderer rewrite, SDK effort, or new data pipeline. The research is aligned: keep mdCAD as the existing native C + Sokol + Dear ImGui application, add a strict Windows-only embedded launch mode that creates a true child window from startup, and let a separate Avalonia sample host launch the viewer as an external process via `NativeControlHost`.

The recommended implementation path is narrow and pragmatic. Add a small launch-config parser for `--embedded`, `--parent-hwnd`, `--jsonl`, and `--jsonl-live-refresh`; add a Windows embedding seam at Sokol's Win32 window-creation boundary; and reuse the current flat JSONL import and linked refresh systems for launch-time auto-open behavior. The host should own process launch, native placeholder HWND, resize, and status reporting. mdCAD should continue owning rendering, scene state, import, refresh, and shutdown semantics.

The main risk is not JSONL handling; it is **correct child-window lifecycle and input behavior** under Win32. The milestone will fail if it relies on late `SetParent`, treats process spawn as "ready," or leaves focus/capture/resize behavior undefined. Mitigation is to absorb risk in order: first true child-window creation and readiness states, then resize/focus/input correctness, then startup import, then live refresh, then sample-host hardening.

## Key Findings

### Recommended Stack

Research strongly favors preserving mdCAD's current native stack and adding only a thin Windows-only embedding path. The Avalonia/.NET work belongs in a separate sample host, not in mdCAD core. This keeps the milestone small, avoids new runtime dependencies in the viewer, and centers effort on the one area that actually changes: Win32 window creation and lifecycle.

**Core technologies:**
- **C + existing mdCAD runtime:** keep the app lifecycle, ECS flow, viewport/input pipeline, and JSONL systems intact.
- **Sokol + Dear ImGui:** keep the current windowing/render loop stack; patch or extend the Win32 creation seam locally for child-window startup.
- **Existing D3D11 / Vulkan backend selection:** keep current renderer selection logic; do not switch renderers for v1.8.
- **Project-local CLI parsing:** parse `--embedded`, `--parent-hwnd`, `--jsonl`, and `--jsonl-live-refresh` without adding a new parsing library.
- **.NET 8 + Avalonia.Desktop 11.3.x sample host:** use `NativeControlHost` plus minimal P/Invoke for HWND discovery, focus, and liveness.

**Critical version / implementation requirements:**
- Pin the vendored Sokol revision for the milestone if the Win32 path is patched locally.
- Require **absolute JSONL paths** in the CLI contract.
- Prefer wide-character argument parsing on Windows to avoid quoting/path failures.

### Expected Features

The feature research is disciplined: v1.8 is a first **Windows-only, process-launched embedded viewer** milestone. It should prove that a host can launch mdCAD into a child region, optionally auto-open a large JSONL dump, and keep resize/focus/input behavior predictable without adding IPC.

**Must have (table stakes):**
- Child-window embedding from a CLI-provided parent HWND, with clear failure on invalid embed args.
- No silent fallback to a standalone/orphan top-level window in embedded mode.
- Launch-time JSONL auto-open from an absolute path.
- Optional `--jsonl-live-refresh` flag, default OFF unless explicitly requested.
- Correct resize behavior inside the host region.
- Click-to-focus keyboard ownership and coherent mouse ownership/capture.
- Clean focus loss/gain behavior and clean teardown on host close.
- Minimal host status messaging that proves launch, attach, import, and failure states.

**Should have (competitive / milestone-completing):**
- Embedded viewer layout mode that lets the viewport fill the hosted region instead of using the normal standalone-heavy layout.
- A minimal Avalonia sample host with bundled example JSONL and explicit build/run instructions.

**Defer (v2+):**
- Rich host-to-viewer commands or IPC.
- Structured machine-readable status channels.
- Multiple embedded mdCAD instances per host.
- Embed-specific UX polish beyond what is needed to prove correctness.
- Drag-and-drop or richer sample-host controls.

### Architecture Approach

Architecture research is consistent: add only three real seams to the existing runtime—**launch config parser, Windows embedding adapter, and startup flat-JSONL import controller**—and keep everything else on the current app lifecycle. `src/app.c` should own startup orchestration, a thin Win32-specific layer should own parent/liveness/focus/sizing glue, and a non-UI startup import helper should reuse the existing import job and observer systems without pushing launch logic into Scene Hierarchy UI code.

**Major components:**
1. **Launch config parser** — parses and validates embedded-mode CLI flags before normal init begins.
2. **Windows embedding adapter** — owns parent HWND validation, embedded-mode state, liveness/orphan detection, and Win32 sizing/focus glue.
3. **Sokol Win32 creation seam** — creates mdCAD as a true child window from birth instead of reparenting after launch.
4. **Startup flat JSONL import controller** — kicks off absolute-path startup import and optional live refresh using existing import/observer semantics.
5. **Avalonia sample host** — owns process lifecycle, placeholder HWND, resize/focus handoff, and minimal status UI only.

### Critical Pitfalls

The pitfall research is the clearest part of the package: most risk concentrates in the early phases and revolves around Win32 lifecycle discipline.

1. **Late `SetParent` reparenting** — avoid it as the final design; create a true child window from startup and validate the parent HWND early.
2. **Treating process launch as viewer readiness** — separate "process launched," "child attached," "first non-zero resize," and "optional import complete" states.
3. **Undefined focus/input ownership** — define click-to-focus, focus-loss cleanup, and host/viewer keyboard ownership explicitly.
4. **Mouse capture / drag leaks across boundaries** — cancel transient interaction state on focus loss, capture loss, resize edge cases, and parent destruction.
5. **Startup import running before embed stability** — delay actual JSONL import until the embedded surface exists and has seen a first non-zero size.
6. **Host/control destruction orphaning mdCAD** — treat parent/control destruction and relaunch cycles as first-class lifecycle cases.

## Implications for Roadmap

Based on combined research, the roadmap should follow the risk-absorption order rather than a feature-marketing order.

### Phase 1: Embed Contract and Child-Window Bootstrap
**Rationale:** Everything else depends on mdCAD starting as a real child window with a strict CLI contract.  
**Delivers:** Launch config parser; `--embedded` + `--parent-hwnd`; parent validation; narrow Sokol Win32 child-window creation path; initial sample host that can launch and display mdCAD.  
**Addresses:** Core embedding promise, explicit failure behavior, no silent standalone fallback.  
**Avoids:** `SetParent` shortcut, false readiness assumptions, invalid-parent ambiguity.

### Phase 2: Resize, Focus, Input, and Embedded Layout Correctness
**Rationale:** Once the window exists, the next make-or-break requirement is that it behaves like a native hosted viewer.  
**Delivers:** Host-driven resize handling; click-to-focus behavior; keyboard/mouse ownership rules; capture-loss cleanup; orphan/parent-close shutdown behavior; embedded viewer layout mode.  
**Addresses:** Resize correctness, keyboard ownership, mouse capture, clean focus transitions, teardown behavior.  
**Uses:** Existing viewport/input pipeline with Win32 glue in a thin platform layer.  
**Avoids:** First-click loss, stuck drags, resize storms breaking viewport math, standalone behavior regressions.

### Phase 3: Startup JSONL Auto-Import
**Rationale:** Import should only be added after the embedded surface and readiness contract are stable.  
**Delivers:** Non-UI startup import controller; absolute-path JSONL auto-open; launch-time success/error flow while keeping the viewer alive on import failure.  
**Implements:** Reuse of the existing flat JSONL import job without routing startup behavior through Scene Hierarchy UI state.  
**Avoids:** Import-too-early races, quoting/path bugs, blank startup on failure.

### Phase 4: Live Refresh Wiring
**Rationale:** Live refresh is valuable but must not destabilize the current observer semantics.  
**Delivers:** `--jsonl-live-refresh`; opt-in-only launch behavior; reuse of existing linked refresh metadata and commit-on-success behavior.  
**Addresses:** Host "watch this dump" scenario.  
**Avoids:** Semantic drift from the existing observer flow, accidental default-on refresh behavior.

### Phase 5: Sample Host Hardening and Documentation
**Rationale:** The sample host should finish as a conformance harness, not a mini-product.  
**Delivers:** Bundled example JSONL; explicit launch/attach/import status messaging; repeated launch/close/reopen coverage; build/run instructions; integration validation notes.  
**Addresses:** End-to-end proof of workflow for downstream consumers.  
**Avoids:** Happy-path-only demos, unclear failure states, zombie process regressions.

### Phase Ordering Rationale

- Phase 1 comes first because every later feature assumes a true child-window contract and reliable readiness detection.
- Phase 2 is intentionally isolated before import work so windowing and interaction bugs are not masked by data-loading behavior.
- Phases 3 and 4 reuse existing JSONL systems, so they should layer on top of a stable embed lifecycle instead of reshaping it.
- Phase 5 closes the loop by turning the host into the validation harness that exercises the real lifecycle and failure matrix.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 1:** Sokol Win32 child-window creation details and any backend-specific quirks under embedded mode still need implementation validation.
- **Phase 2:** Focus, mouse activation/capture, and resize edge cases will likely need targeted Windows behavior verification during planning/execution.

Phases with standard / well-supported patterns:
- **Phase 3:** Startup import is mostly reuse of existing mdCAD import behavior once launch gating is defined.
- **Phase 4:** Live refresh should follow the already-established observer contract; the main requirement is restraint, not new design.
- **Phase 5:** Sample-host packaging and documentation are straightforward once lifecycle expectations are fixed.

## Planning Guidance / Non-Goals

Plan this milestone as a **strict CLI-driven external-process embedding** effort. Keep the host/viewer boundary narrow: the host supplies parent HWND, process lifetime, resize, and status; mdCAD owns rendering, scene state, import, refresh, and shutdown. Use explicit failure modes instead of compatibility fallbacks.

Do not let planning expand scope into:
- in-process / DLL / SDK embedding
- rich IPC or host command surfaces
- renderer rewrites or backend swaps
- cross-platform embedding parity
- refresh architecture redesign
- folding Avalonia/.NET into mdCAD core
- turning the sample host into a product-grade UI

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Strong convergence across repo inspection and official Win32/Avalonia/Sokol constraints; main uncertainty is local Sokol patch shape. |
| Features | MEDIUM | Scope is clear and disciplined, but host UX expectations and exact polish threshold are less formally sourced. |
| Architecture | MEDIUM | The seams and ownership boundaries are well reasoned, but the Windows embedding path still needs proof in code. |
| Pitfalls | HIGH | Risks are concrete, phase-mapped, and backed by known Win32 lifecycle/input failure modes. |

**Overall confidence:** MEDIUM

### Gaps to Address

- **Exact Sokol Win32 modification shape:** validate the least-invasive child-window creation approach during Phase 1 design spikes.
- **Backend behavior in embedded mode:** confirm no material D3D11/Vulkan differences appear once mdCAD renders as a child HWND.
- **Readiness signaling details in the sample host:** define the minimum attach/ready timeout and status model during planning.
- **Embedded layout policy specifics:** verify how much standalone chrome must be suppressed to keep the hosted viewport credible without creating a forked UI path.

## Sources

### Primary (HIGH confidence)
- `src/app.c`, `src/platform.h`, `src/jsonl_import_job.h`, `src/jsonl_observer_system.h`, `src/ui/ui_viewport.h`, `src/ui/ui_scene_hierarchy.h` — existing runtime integration points and reuse boundaries.
- `vendors/libsokol/sokol.c`, Sokol `sokol_app.h` — native window creation boundary and current app-hosting constraints.
- Microsoft Win32 docs: `CreateWindowExW`, `SetParent`, `EnumChildWindows`, `GetWindowThreadProcessId`, `SetFocus`, `SetCapture`, `GetClientRect`, `WM_SIZE`, `WM_MOUSEACTIVATE`, `CreateProcessW`, `WaitForInputIdle`, `CommandLineToArgvW` — lifecycle, embedding, focus, sizing, and process-launch behavior.
- Avalonia `NativeControlHost` docs and Win32 host implementation — native host surface behavior for the sample application.

### Secondary (MEDIUM confidence)
- `.planning/research/STACK.md` — stack recommendation and CLI contract.
- `.planning/research/FEATURES.md` — milestone table stakes, defer list, and anti-features.
- `.planning/research/ARCHITECTURE.md` — recommended seams, ownership boundaries, and phase order.
- `.planning/research/PITFALLS.md` — risk map and prevention strategy by phase.
- `.planning/PROJECT.md`, `.planning/STATE.md`, `.planning/milestones/v1.6-REQUIREMENTS.md`, `.planning/milestones/v1.7-REQUIREMENTS.md` — project/milestone continuity context cited by feature and pitfall research.

---
*Research completed: 2026-05-14*  
*Ready for roadmap: yes*

# Milestone v1.9: Plain net10 Avalonia Host Compatibility

**Status:** ACTIVE  
**Phases:** 50-54, plus inserted Phase 52.1  
**Total Plans:** 15

## Overview

v1.9 widens the **host-facing compile-time contract** for the reusable Avalonia control so a plain `net10.0` host can reference it directly, while keeping the actual mdCAD embedded viewer runtime **Windows-only**.

The roadmap absorbs risk in the order the research recommended: first isolate the existing Windows runtime seam behind a backend boundary, then make non-Windows behavior explicit, then widen the public control TFM, then automate the Windows runtime refresh path, then prove the new consumer path while re-closing the Windows harness, and finally make the docs/onboarding contract truthful.

## Phases

### Phase 50: Backend Seam Extraction and Windows Behavior Lock

**Goal:** Isolate Windows-only placeholder, attach, resize, runtime lookup, and process-launch logic behind an internal backend seam without changing the shipped Windows runtime behavior.  
**Depends on:** Phase 49  
**Plans:** 3 plans  
**Requirements:** WPRS-01, WPRS-02  
**Status:** Complete

Plans:
- [x] 50-01-PLAN.md — Lock Windows runtime lookup/launch behavior with backend-proof tests, seam contract, and manual checklist scaffold
- [x] 50-02-PLAN.md — Extract the Windows backend implementation and thin the shared control shell without changing behavior
- [x] 50-03-PLAN.md — Re-close automated/manual Windows regression proof on the existing diagnostic host

**Success Criteria:**
1. Shared control shell no longer owns Win32 placeholder/runtime-launch logic directly.
2. Windows diagnostic harness still launches, attaches, stops, and relaunches through the existing child-HWND contract.
3. Windows runtime bundle lookup plus sealed/diagnostic behavior remain intact after the refactor.

### Phase 51: Unsupported-Platform Contract

**Goal:** Make non-Windows behavior intentional, visible, and safe instead of relying on Windows-only seams failing implicitly.  
**Depends on:** Phase 50  
**Plans:** 3 plans  
**Requirements:** PLAT-01, PLAT-02, PLAT-03  
**Status:** Complete

Plans:
- [x] 51-01-PLAN.md — Add Wave 0 unsupported-backend and coordinator contract tests in the existing Windows-targeted test project
- [x] 51-02-PLAN.md — Implement the internal unsupported backend, backend factory, canonical message, and blocked-reason coordinator seam
- [x] 51-03-PLAN.md — Wire unsupported selection into the control shell and surface truthful sealed/diagnostic unsupported messaging

**Success Criteria:**
1. Non-Windows hosts render a clear placeholder/warning instead of a blank or broken embed surface.
2. `AutoStart` and launch-affecting property changes never attempt mdCAD launch on unsupported platforms.
3. `StartAsync()` / `StopAsync()` expose explicit, stable unsupported-platform behavior.

### Phase 52: Plain net10 Control Compatibility

**Goal:** Widen the public control contract to plain `net10.0` so cross-platform Avalonia hosts can reference and instantiate the control without the current TFM compatibility wall.  
**Depends on:** Phase 51  
**Plans:** 3 plans  
**Requirements:** HOSTC-01, HOSTC-02  
**Status:** Complete

Plans:
- [x] 52-01-PLAN.md — Retarget the reusable control library to plain `net10.0` without changing the internal Windows runtime seam
- [x] 52-02-PLAN.md — Retarget `samples/avalonia-host-minimal` as the D-01 plain-net10 proof host and manually smoke shared XAML control instantiation
- [x] 52-03-PLAN.md — Re-close the existing control test lane and Windows diagnostic harness against the widened contract

**Success Criteria:**
1. A plain `net10.0` host can restore/build against the control without `NU1201`.
2. The control can be instantiated from shared XAML/code in the plain-`net10.0` proof host without startup crash.
3. The Windows diagnostic harness still builds against the widened public contract.

### Phase 52.1: Automate Windows runtime refresh from build-vulkan with a dotnet-managed post-build helper (INSERTED)

**Goal:** Add a dotnet-managed C# build helper and control-project post-build flow that rebuilds mdCAD in `build-vulkan` and refreshes the committed Windows runtime payload under `samples/avalonia-mdcad-control/runtime/win-x64` without relying on PowerShell.  
**Depends on:** Phase 52  
**Plans:** 3 plans  
**Requirements:** WPRS-04  
**Status:** Complete

Plans:
- [x] 52.1-01-PLAN.md — Add the dotnet-managed runtime-refresh helper and lock path/copy-policy tests
- [x] 52.1-02-PLAN.md — Wire opt-in control-project post-build refresh for the committed bundle plus current output
- [x] 52.1-03-PLAN.md — Close integration with host build and hash/path verification of the refreshed runtime chain

**Success Criteria:**
1. The repo has a .NET/C# helper flow that can rebuild mdCAD from the Windows Vulkan build path and refresh the runtime bundle without PowerShell.
2. The Avalonia control project can trigger that helper from a post-build path instead of depending on manual runtime refresh steps.
3. The refreshed runtime lands in `samples/avalonia-mdcad-control/runtime/win-x64` so later proof and docs phases consume the maintained bundle contract.

### Phase 53: Consumer Proof and Windows Regression Closure

**Goal:** Prove the new host-facing compatibility path while re-verifying that the already-shipped Windows runtime path still copies, attaches, stops, and relaunches correctly.  
**Depends on:** Phase 52.1  
**Plans:** 3 plans  
**Requirements:** WPRS-03, PROOF-01  
**Status:** Executing

Plans:
- [x] 53-01-PLAN.md — Lock `samples/avalonia-host-minimal` as the sole plain-net10 PROOF-01 build surface
- [x] 53-02-PLAN.md — Re-close the automated Windows regression preflight from the existing host build plus targeted tests
- [ ] 53-03-PLAN.md — Create and complete a new Phase 53 manual lifecycle proof artifact on `samples/avalonia-host`

**Success Criteria:**
1. Repository includes a plain `net10.0` consumer proof that builds successfully against the control.
2. Windows proof harness still verifies copied-runtime presence plus attach/stop/relaunch lifecycle.
3. Regression coverage clearly distinguishes plain-host compile proof from real Windows runtime proof.

### Phase 54: Docs and Onboarding Truthfulness

**Goal:** Make the compile-time vs runtime support contract explicit in docs, samples, and onboarding proof so consumers understand exactly what v1.9 delivers.  
**Depends on:** Phase 53  
**Plans:** 0 planned  
**Requirements:** PROOF-02  
**Status:** Pending

**Success Criteria:**
1. Quickstart/README clearly separate compile-time host compatibility from runtime viewer support.
2. Sample-host wording reflects Windows-only runtime support truthfully.
3. Final onboarding/proof checklist covers both plain-host and Windows-runtime expectations.

## Progress

| Phase | Status | Plans | Progress |
|-------|--------|-------|----------|
| 50 | ✓ | 3/3 complete | 100% |
| 51 | ✓ | 3/3 complete | 100% |
| 52 | ✓ | 3/3 complete | 100% |
| 52.1 | ✓ | 3/3 complete | 100% |
| 53 | ◐ | 2/3 complete | 67% |
| 54 | ○ | 0 planned | 0% |

## Coverage

- Requirements mapped: 11 / 11
- Unmapped requirements: 0 ✓

## Next Step

- Execute `/gsd-execute-phase 53`

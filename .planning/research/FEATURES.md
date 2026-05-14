# Feature Landscape

**Project:** mdCAD v1.8 — Embeddable Windows JSONL Viewer  
**Domain:** Windows-hosted, process-launched child-HWND viewer workflow  
**Researched:** 2026-05-14  
**Confidence:** MEDIUM

## Scope Anchor

v1.8 is a first **Windows-only, process-launched, child-HWND embedding** milestone for an Avalonia host using `NativeControlHost`.

The goal is not "embedding mdCAD everywhere." The goal is a credible first workflow where a Windows host launches mdCAD into a native child surface, optionally auto-opens a JSONL large dump, and keeps resize/focus/input behavior predictable without adding IPC.

---

## Table Stakes

These are the expected must-haves for a first embedded viewer release.

| Feature | Why it matters | Notes |
|---------|----------------|-------|
| Child-window embedding by CLI-provided parent HWND | This is the core promise of the milestone | Invalid/missing parent handle must fail clearly |
| No fallback to an orphan top-level window | Silent fallback would hide integration bugs | Embedded mode should be explicit and strict |
| Launch-time JSONL auto-open from an absolute path | Matches the requested host workflow | Reuse the existing large flat dump import path |
| Optional live refresh flag | Supports the host "watch this dump" scenario | Keep default OFF unless explicitly requested |
| Resize follows the host control cleanly | Embedded viewer must feel native inside the host | No stale backbuffer, clipped viewport, or delayed resize behavior |
| Click-to-focus keyboard ownership | Viewer shortcuts must work when the user interacts with the viewer | Host should retain shortcuts when viewer is not focused |
| Mouse ownership stays local to the viewer | Orbit/pan/select/zoom must not leak into host scrolling/gestures | Capture/release behavior matters |
| Clean focus loss/gain behavior | Switching between host UI and viewer must feel normal | No stuck drag or broken keyboard state |
| Minimal host status messaging | The sample host should prove the workflow | Launching, embedded, JSONL path, live refresh, failure states |
| Clean teardown with host close | No zombie mdCAD process or detached window | Parent/control destruction is part of the feature contract |

## Expected v1.8 Behavior

### Launch and embedding
- Host launches `mdCAD.exe` as a separate process.
- Host passes a parent HWND on the command line.
- mdCAD creates/attaches a real child window beneath that parent.
- Embedded startup failure is surfaced as a failure, not a hidden standalone fallback.

### Startup JSONL behavior
- If a JSONL path is supplied, mdCAD auto-imports it on startup.
- If live refresh is also requested, it enables linked refresh for that imported file only.
- If no JSONL path is supplied, mdCAD still starts as an empty embedded viewer.

### Input behavior
- Clicking inside the embedded viewer gives mdCAD focus.
- Keyboard input belongs to mdCAD only while its surface is focused.
- Mouse drag/capture must remain coherent during viewport interaction.
- Mouse wheel over the viewer should affect the viewer, not the outer host surface.

### Resize behavior
- Resizing the Avalonia `NativeControlHost` should resize the mdCAD child surface immediately.
- Viewport rendering, picking, and interaction math should remain correct after repeated resizes.

---

## Defer to Later Milestones

These are useful later, but should not block v1.8:

| Deferred item | Why defer |
|---------------|-----------|
| Rich host-to-viewer commands (reload, fit-to-view, frame selection) | Requires an IPC/control surface the milestone explicitly excludes |
| Structured machine-readable status channel | Helpful later, but not required for a first CLI-only workflow |
| Multiple embedded mdCAD instances per host | Extra lifetime/focus complexity before the single-view story is stable |
| Embed-specific chrome trimming or read-only viewer UX | Nice polish, but not necessary to prove correctness |
| Drag-and-drop JSONL onto the embedded viewer | Outside the requested launch contract |
| Rich sample-host controls (path picker, reload button, live toggle) | Risks turning the sample into a product rather than a conformance harness |

---

## Anti-Features

These should stay explicitly out of scope for v1.8:

| Anti-feature | Why avoid it |
|--------------|--------------|
| In-process / DLL / SDK-style embedding | Major architecture expansion with much higher lifetime and ABI risk |
| Rich IPC between host and viewer | Expands the contract before the basic embedding story is stable |
| Cross-platform embedding parity | Multiplies platform complexity too early |
| Refresh architecture redesign | Existing linked flat JSONL refresh behavior is already the stable base to reuse |
| Full host/viewer state synchronization | Scope explosion far beyond "embedded viewer" |
| Automatic fallback to standalone mode on bad embed args | Hides bugs and makes the host workflow unreliable |

---

## MVP Recommendation

Build in this order:

1. Valid embedded child-window launch
2. Resize/focus/input correctness
3. Launch-time JSONL auto-import
4. Optional live refresh wiring
5. Minimal Avalonia sample host with bundled example JSONL

---

## "Done Means"

v1.8 should feel done when all of these are true:

- A Windows Avalonia host can launch mdCAD and host it inside `NativeControlHost`
- mdCAD remains visually confined to the host region as a child window
- An absolute JSONL path auto-loads on startup
- A separate flag can opt into live refresh for that file
- Resize/focus/keyboard/mouse behavior feels normal during real viewport interaction
- The sample host clearly communicates launch/attach/import state and demonstrates the full workflow end-to-end

---

## Sources

- `.planning/PROJECT.md`
- `.planning/STATE.md`
- `.planning/milestones/v1.6-REQUIREMENTS.md`
- `.planning/milestones/v1.7-REQUIREMENTS.md`
- Microsoft `SetParent`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setparent
- Microsoft child-window docs: https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features
- Microsoft `WM_SETFOCUS`: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-setfocus
- Avalonia `NativeControlHost`: https://api-docs.avaloniaui.net/docs/T_Avalonia_Controls_NativeControlHost

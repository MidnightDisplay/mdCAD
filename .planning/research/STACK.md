# Technology Stack

**Project:** mdCAD v1.8 — Embeddable Windows JSONL Viewer  
**Domain:** Windows child-HWND embedding, CLI startup import, and Avalonia sample hosting  
**Researched:** 2026-05-14  
**Confidence:** HIGH on stack direction, MEDIUM on backend-specific embedding quirks

## Recommendation in One Line

Keep mdCAD as a native C/Sokol/Dear ImGui application, add a Windows-only embedded child-window path at the Win32 window-creation seam, and put all Avalonia/.NET work in a separate sample host project.

---

## Recommended Stack Changes

| Area | Recommendation | Why |
|------|----------------|-----|
| mdCAD core | Add a small launch-config parser for `--embedded`, `--parent-hwnd`, `--jsonl`, and `--jsonl-live-refresh` | No new dependency is needed; `sokol_main(int argc, char* argv[])` already receives args |
| mdCAD windowing | Add a Windows-only embedded mode that creates a real `WS_CHILD` window from startup | Child-window creation is the core platform requirement |
| Startup import | Reuse the existing large flat JSONL import path | Milestone scope is launch-time wiring, not a new import system |
| Sample host | Use a separate Avalonia desktop project with `NativeControlHost` | Keeps .NET/Avalonia out of mdCAD core |
| Host-side Win32 glue | Use minimal P/Invoke for HWND discovery/lifetime (`EnumChildWindows`, `GetWindowThreadProcessId`, `SetFocus`, `IsWindow`) | Enough to host and manage the child process without IPC |
| Build/test infra | Pin the vendored Sokol revision for this milestone and add Windows embedding smoke coverage | Avoids tracking `master` while locally extending Win32 creation behavior |

---

## mdCAD Core

### Keep unchanged
- C application/runtime
- Sokol + Dear ImGui stack
- Existing Windows backend selection (`D3D11` / `Vulkan`)
- Existing JSONL import and linked refresh code

### Add
- Windows-only embedded startup mode
- Project-local CLI parsing
- Launch-time JSONL auto-import hook

### Important conclusion
This milestone is primarily a **windowing/bootstrap problem**, not a renderer rewrite and not a JSONL parser problem.

---

## Windowing Recommendation

### Prefer a narrow local Sokol Win32 patch/extension

mdCAD already depends on Sokol for:
- frame loop
- native window lifecycle
- event delivery
- graphics setup

The recommended path is a **small Windows-only project-owned extension** at the Win32 creation seam so embedded mode can create a child window from birth.

### Why not late `SetParent` as the main design?
- Win32 child/top-level styles are easy to get wrong after creation
- Focus/activation tends to be less reliable
- It risks a visible standalone-window flash during launch
- Microsoft explicitly documents `SetParent` caveats around styles and DPI

### Minimal embedded-mode shape
- `embedded = true`
- `parent_hwnd = <HWND>`
- child window styles such as `WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN`
- initial size from the parent client rect
- no standalone-only decorations/fullscreen behaviors in embedded mode

---

## CLI Contract Recommendation

Use a small, explicit flag surface:

```bash
mdCAD.exe --embedded --parent-hwnd 0x12345678
mdCAD.exe --embedded --parent-hwnd 0x12345678 --jsonl "C:\abs\path\dump.jsonl"
mdCAD.exe --embedded --parent-hwnd 0x12345678 --jsonl "C:\abs\path\dump.jsonl" --jsonl-live-refresh
```

Rules:
- `--embedded` enables the Windows-only child-host path
- `--parent-hwnd` is required when embedded mode is used
- `--jsonl` accepts absolute paths only
- `--jsonl-live-refresh` is an explicit boolean opt-in
- bad HWND / missing parent / relative path should fail clearly, not silently

---

## Sample Host Stack

### Recommended sample project
- .NET 8
- Avalonia.Desktop 11.3.x
- `NativeControlHost`
- minimal Win32 P/Invoke helpers

### Keep out of mdCAD core
Do **not** pull these into mdCAD itself:
- C#
- .NET runtime requirements
- Avalonia packages
- managed/native bridge code

### Recommended host flow
1. `NativeControlHost` obtains a native parent HWND
2. Host launches `mdCAD.exe --embedded --parent-hwnd <HWND> ...`
3. Host enumerates child HWNDs under the placeholder and filters by mdCAD process ID
4. Host attaches the discovered child window to the native control handle
5. Host owns process lifetime, bounds, and status messaging

This keeps the milestone CLI-driven only while still making the sample credible.

---

## Backend Impact

The new risk is the Win32 hosting seam, not the existing graphics backend selection.

Recommendation:
- Do **not** switch renderers for v1.8
- Keep current backend selection logic intact
- Make embedded mode work with the same backend the current build already uses

Only revisit backend-specific differences if child-window rendering proves materially different during implementation.

---

## Build and Verification

### mdCAD
- Keep existing CMake build
- Add no new native runtime/language dependency
- Pin Sokol during the milestone if the Win32 path is patched locally

### Sample host
- Build separately with `dotnet build`
- Do not fold Avalonia into the native CMake build
- Keep sample-host build instructions explicit and local to the sample

### Verification
Add Windows-focused validation for:
- embedded launch
- child-window attach
- resize
- focus
- keyboard
- mouse capture/release
- startup JSONL import
- optional live refresh
- host close / child shutdown

---

## Explicit Non-Goals

- No IPC layer
- No in-process embedding
- No DLL/SDK packaging
- No renderer rewrite
- No new CLI parsing library
- No WPF/WinUI alternative sample
- No cross-platform sample host in this milestone

---

## Sources

- `src/app.c`
- `src/platform.h`
- `vendors/libsokol/CMakeLists.txt`
- `vendors/libsokol/sokol.c`
- Sokol `sokol_app.h`: https://raw.githubusercontent.com/floooh/sokol/master/sokol_app.h
- Avalonia `NativeControlHost`: https://raw.githubusercontent.com/AvaloniaUI/Avalonia/master/src/Avalonia.Controls/NativeControlHost.cs
- Avalonia Win32 host implementation: https://raw.githubusercontent.com/AvaloniaUI/Avalonia/master/src/Windows/Avalonia.Win32/Win32NativeControlHost.cs
- Avalonia.Desktop package metadata: https://api.nuget.org/v3-flatcontainer/avalonia.desktop/11.3.15/avalonia.desktop.nuspec
- Microsoft `CreateWindowExW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw
- Microsoft `EnumChildWindows`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumchildwindows
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
- Microsoft `SetFocus`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setfocus
- Microsoft `SetParent`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setparent

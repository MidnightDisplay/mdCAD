# Phase 48: Reusable Avalonia mdCAD user control - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `48-CONTEXT.md` — this log preserves the alternatives considered.

**Date:** 2026-05-15
**Phase:** 48-reusable-avalonia-mdcad-user-control
**Areas discussed:** control startup contract, runtime payload packaging, control presentation modes, JSONL path fallback behavior

---

## Startup mode

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-launch when the control gets its native host handle | Match the user's preferred "drop it in and let it load" path immediately | |
| Require the host app to call an explicit start method | Keep startup fully host-driven | |
| Support both auto-launch and explicit start | Default-friendly for simple hosts while preserving a delayed-start escape hatch | ✓ |

**User's choice:** Support both auto-launch and explicit start.
**Notes:** The reusable control should not force every host into one launch style.

---

## Post-launch setting changes

| Option | Description | Selected |
|--------|-------------|----------|
| Startup-only | Changes apply only before the first launch | |
| Automatically relaunch mdCAD with the new startup args | Control owns the restart when launch-affecting settings change | ✓ |
| Allow changes only through an explicit restart call | Host app owns restart timing | |

**User's choice:** Changing launch-affecting settings should automatically relaunch mdCAD with the new startup args.
**Notes:** The control should own the runtime restart mechanics rather than pushing that burden into consuming apps.

---

## External configuration surface

| Option | Description | Selected |
|--------|-------------|----------|
| Bindable XAML/control properties | Natural fit for a reusable Avalonia control dropped into layout markup | ✓ |
| Constructor/options object only | Programmatic-only setup path | |
| Both bindable properties and a programmatic options object | Support both UI and pure-code configuration styles | |

**User's choice:** Bindable XAML/control properties on the reusable control.
**Notes:** The user explicitly wants to drop the control into a grid/stack panel and configure it from the owning app.

---

## Default launch behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-launch by default; explicit start is advanced/opt-out | Best fit for host-app-launch startup driven from stored settings | ✓ |
| Stay idle by default; host opts into auto-launch | Conservative default requiring extra host setup | |
| No default; host must always choose explicitly | Forces every consumer to wire startup behavior manually | |

**User's choice:** Auto-launch by default; explicit start is the advanced path.
**Notes:** This matches the user's preferred host-app-launch workflow.

---

## Runtime payload source

| Option | Description | Selected |
|--------|-------------|----------|
| Repo-owned pinned/stable native bundle copied into the client app output | Deterministic, consumer-friendly packaging with no repo-local build discovery | ✓ |
| Build mdCAD from source during the client app build | Consumer build drives native compilation | |
| Require the consuming app to supply an external mdCAD path manually | Packaging burden stays on the host app | |

**User's choice:** A repo-owned pinned/stable native bundle that the control project copies into the client app output.
**Notes:** The user wants a stable mdCAD build passed down to the client app automatically.

---

## Bundled payload contents

| Option | Description | Selected |
|--------|-------------|----------|
| Full mdCAD runtime folder | `mdCAD.exe` plus every sibling DLL/asset/config file it needs | ✓ |
| Only `mdCAD.exe` | Consuming app manages all companion files manually | |
| `mdCAD.exe` plus a small hand-picked subset | Partial runtime bundle curated by the project | |

**User's choice:** A full mdCAD runtime folder.
**Notes:** This is the simplest way to keep runtime path resolution predictable in external apps.

---

## Consumer output layout

| Option | Description | Selected |
|--------|-------------|----------|
| Dedicated subfolder under the client app output | Stable internal resolution point and cleaner consumer output root | ✓ |
| Directly beside the client app executable | Flat output layout | |
| Configurable per consuming app | Host decides where the runtime folder lands | |

**User's choice:** A dedicated subfolder under the client app output.
**Notes:** The control should not depend on repo layout or pollute the consumer app's root output folder.

---

## Runtime path override policy

| Option | Description | Selected |
|--------|-------------|----------|
| Always use the copied stable bundle | Control owns runtime resolution completely | ✓ |
| Allow an optional override path for development/testing | Host can bypass the copied runtime folder | |
| Require the consuming app to tell the control which runtime folder to use | Host owns runtime resolution entirely | |

**User's choice:** Always use the copied stable bundle.
**Notes:** The user does not want Phase 48 to depend on external path overrides.

---

## Control presentation mode

| Option | Description | Selected |
|--------|-------------|----------|
| `sealed` by default with opt-in `diagnostic` mode | Bare viewer normally; sample-host controls/status surface only when the owning app explicitly opts in | ✓ |
| Always diagnostic | Always show the sample-host controls/status surface | |
| Always sealed | Never expose the extra host chrome/status surface | |

**User's choice:** Two modes: diagnostic = "all the guts out", sealed = default.
**Notes:** Diagnostic mode should append the sample-host-style controls and status/failure bits above the mdCAD native control. Sealed mode stays bare except for path-problem warnings.

---

## Invalid `JsonlPath` behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Show a warning and still launch an empty usable viewer | Preserve viewer usability while surfacing the bad requested path | ✓ |
| Block launch until the path is corrected | No viewer until the path is valid | |
| Launch without `--jsonl` silently | Hides the bad requested path from the user | |

**User's choice:** Show a warning and still launch an empty usable viewer.
**Notes:** The user wants the viewer to remain usable even when the requested startup file is bad.

---

## Unset/empty `JsonlPath` behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Launch mdCAD normally with no warning and no `--jsonl` | Treat the path as truly optional | ✓ |
| Show the same visible warning as a bad path | Warn even when no path was requested | |
| Stay idle until a path is supplied | Make JSONL path effectively required for startup | |

**User's choice:** Launch mdCAD normally with no warning and no `--jsonl`.
**Notes:** The control should distinguish "no file requested" from "bad file requested."

---

## the agent's Discretion

- Exact Avalonia property names and their bindable implementation details
- Exact dedicated runtime-subfolder name
- Exact relaunch debounce/restart orchestration when multiple bound properties change quickly
- Exact sealed-mode warning visual treatment
- Exact MSBuild targets/items used to copy the pinned runtime bundle into consuming app outputs

## Deferred Ideas

- NuGet packaging later
- Rich IPC / machine-readable runtime status events
- Multi-viewer hosting / broader orchestration
- Cross-platform reusable host-control parity

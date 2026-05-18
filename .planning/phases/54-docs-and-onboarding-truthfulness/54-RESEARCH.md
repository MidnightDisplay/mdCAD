# Phase 54: Docs and Onboarding Truthfulness - Research

**Researched:** 2026-05-18
**Domain:** Avalonia embed-contract docs, onboarding truth, and proof-surface alignment
**Confidence:** HIGH

<user_constraints>
## User Constraints

### Source
No `54-CONTEXT.md` exists. These constraints come from the phase prompt and current milestone state.

### Locked Decisions
- Phase 54 is primarily a docs/onboarding phase; do not reopen Phase 52 / 52.1 / 53 implementation scope.
- Preserve the already-proven separation between compile/build proof and real Windows runtime proof.
- The reusable Avalonia control and minimal proof host now target plain `net10.0`.
- The embedded mdCAD runtime itself remains Windows-only.
- Phase 53 is already complete and remains the authority for:
  1. plain `net10.0` build proof on `samples/avalonia-host-minimal`
  2. automated Windows preflight on `samples/avalonia-host` plus existing tests
  3. approved manual runtime lifecycle proof on `samples/avalonia-host`

### the agent's Discretion
- Determine every user-facing doc/onboarding/proof surface that materially affects the compile-time vs runtime support story.
- Decide whether any sample-host file needs a minimal wording/sample-data tweak for truthfulness.
- Propose the executable Phase 54 plan split (likely 3 plans unless repo evidence says otherwise).
- Define the validation architecture that proves docs and proof surfaces match the real support boundary.

### Deferred Ideas (OUT OF SCOPE)
- New runtime capabilities or cross-platform embedded viewer support
- Reopening backend/runtime implementation work from Phases 50-53
- Broad cleanup of unrelated build docs such as the general `docs/QUICKSTART.md`, unless Phase 54 evidence shows it materially affects the Avalonia support contract
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PROOF-02 | Quickstart/README clearly separate compile-time host compatibility from runtime viewer support and document unsupported-platform behavior truthfully. | Update README + control QUICKSTART to reflect actual TFMs, keep minimal host as compile/build proof only, keep diagnostic host as Windows runtime proof, and document the canonical unsupported-platform behavior already implemented/tested in the control. |
</phase_requirements>

## Summary

Phase 54 is a repo-truth alignment phase, not a new runtime phase. The implementation boundary is already settled by Phases 51-53: `MdCad.Avalonia.Control` compiles for plain `net10.0`, `samples/avalonia-host-minimal` is the compile/build proof surface for that widened contract, and the real embedded viewer runtime is still Windows-only and only fully proved through the Windows-targeted diagnostic host plus the approved manual Phase 53 checklist.

The current gap is documentation drift. `README.md` and `samples/avalonia-mdcad-control/QUICKSTART.md` still contain pre-Phase-52 language that says hosts must target `net10.0-windows10.0.19041.0`, even though the actual control project targets plain `net10.0` and the minimal proof host also targets plain `net10.0`. Those same docs also fail to explain the already-implemented non-Windows behavior: the host/control remains valid, but the embedded viewer will not launch, the control shows a canonical Windows-only warning, and requested JSONL/live-refresh inputs remain informational only on unsupported platforms.

**Primary recommendation:** Plan Phase 54 as three docs-first waves: (1) fix README contract wording, (2) fix control quickstart + minimal sample onboarding truth, and (3) lock the story with validation/proof artifacts that explicitly preserve the compile-proof vs Windows-runtime-proof split.

## Project Constraints (from copilot-instructions.md)

None — `copilot-instructions.md` is not present at the repo root.

## Truth Gaps and Contradictions

| Surface | Current repo wording / behavior | Repo truth | Confidence |
|---------|----------------------------------|------------|------------|
| `README.md:16` | Says the reusable control is for `net10.0-windows10.0.19041.0` Avalonia hosts. | `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` now targets plain `net10.0`. | HIGH |
| `README.md:48-55` | Says both the reusable control and sample hosts target `net10.0-windows10.0.19041.0`. | Minimal host is plain `net10.0`; only the diagnostic host remains Windows-targeted. | HIGH |
| `README.md:48-55` | Presents the minimal sample with `dotnet build` + `dotnet run`, which blurs compile/build proof with runtime support. | Phase 53 locked `samples/avalonia-host-minimal` as compile/build proof; real runtime proof lives on `samples/avalonia-host`. | HIGH |
| `samples/avalonia-mdcad-control/QUICKSTART.md:3-6` | Says consumer host project should target `net10.0-windows10.0.19041.0` and says the reusable control stays Windows-only. | Compile-time host compatibility is plain `net10.0`; only embedded runtime launch is Windows-only. | HIGH |
| `samples/avalonia-mdcad-control/QUICKSTART.md:24-29` and `samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs:5-7` | Uses a developer-specific `GeoMate` JSONL path as if it were a reusable onboarding example. | Fresh consumers cannot rely on that path; it should be replaced, neutralized, or explicitly marked as a replace-me placeholder. | HIGH |
| Unsupported-platform docs | No primary onboarding doc explains what happens on non-Windows. | Canonical behavior already exists: `MdCadUnsupportedRuntime.UnsupportedRuntimeMessage`, disabled start, `launch: unsupported`, `attach: unsupported`, informational-only JSONL/live-refresh status. | HIGH |

## Likely In-Scope Files

| Path | Why it is in scope |
|------|--------------------|
| `README.md` | Primary external onboarding surface; currently contains stale host/runtime contract wording. |
| `samples/avalonia-mdcad-control/QUICKSTART.md` | Primary consumer wiring guide; currently contains stale TFM guidance and a misleading sample path. |
| `samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs` | User-facing sample code; contains a personal absolute JSONL path that weakens onboarding truthfulness. |
| `.planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md` | Needed to lock exact doc-proof commands for this phase. |
| Possibly `.planning/phases/54-docs-and-onboarding-truthfulness/54-*-PLAN.md` follow-on plan files | Needed once planning starts. |

## Explicitly Out of Scope

| Path / Area | Why it should stay untouched |
|-------------|------------------------------|
| `docs/QUICKSTART.md` | Contains broad build history and no Avalonia/control/runtime-support guidance from the Phase 54 grep audit. |
| `samples/avalonia-mdcad-control/Host/*` runtime implementation | Runtime behavior is already implemented and tested; Phase 54 is not a backend phase. |
| `samples/avalonia-host/AvaloniaHost.csproj` and core lifecycle code | The Windows runtime proof surface is already correct; docs should describe it, not redesign it. |
| Historical Phase 50/53 proof outcomes | Use them as sources of truth; do not rewrite history to solve Phase 54. |

## Recommended Phase Breakdown

### Plan 1: README contract rewrite
**Goal:** Make the top-level README distinguish compile-time host compatibility from runtime viewer support without changing code.

**Do:**
- Rewrite the embedding-related README bullets and quickstart subsections so they say:
  - the control can be referenced from a plain `net10.0` Avalonia host
  - the minimal host is the compile/build proof surface
  - the diagnostic host is the Windows runtime proof surface
  - the embedded viewer remains Windows-only at runtime
- Remove or reframe README wording that implies both sample hosts are Windows-targeted.
- Reword the minimal-host section so `dotnet build` is the authoritative proof step and any `dotnet run` text does not overpromise cross-platform runtime support.

**Files:** `README.md`

### Plan 2: Control quickstart + minimal sample onboarding cleanup
**Goal:** Align the reusable-control quickstart and the minimal sample with the real v1.9 contract.

**Do:**
- Update `samples/avalonia-mdcad-control/QUICKSTART.md` to say the host project can target plain `net10.0`.
- Add an explicit unsupported-platform note sourced from the implemented control behavior.
- Replace or neutralize the developer-specific JSONL sample path in the doc snippet.
- Decide the smallest truthful sample-code fix for `samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs`:
  - preferred: replace the personal path with an obvious placeholder or a no-file default
  - acceptable only if clearly documented: keep the code but mark it as a local replace-me path

**Files:** `samples/avalonia-mdcad-control/QUICKSTART.md`, likely `samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs`

### Plan 3: Validation/proof lock for onboarding truth
**Goal:** Prove the docs match the actual support boundary and keep the compile-proof/runtime-proof split explicit.

**Do:**
- Add Phase 54 validation rows for:
  - README/QUICKSTART wording presence
  - plain `net10.0` build proof on the minimal host
  - Windows runtime proof references staying on the diagnostic host + existing tests/checklist
- Prefer a phase-local validation artifact over mutating older checklists.
- If a phase-local onboarding checklist is needed, make it reference Phase 53 proof artifacts instead of replacing them.

**Files:** `.planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md` (and only if needed, a new 54-local checklist/proof note)

**Recommended split:** 3 plans. This keeps README changes, sample/quickstart cleanup, and validation/proof closure separate without reopening runtime implementation.

## Standard Stack

### Core

| Library / Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| .NET SDK | 10.0.300 | Builds all Avalonia proof/doc surfaces | Installed locally and used for package inspection during research. |
| Avalonia | 11.3.15 | Shared control and sample-host UI stack | Actual resolved package version in both host samples. |
| Avalonia.Desktop | 11.3.15 | Desktop runtime for both sample hosts | Required by both proof hosts. |
| `MdCad.Avalonia.Control` | repo project, TFM=`net10.0` | Reusable consumer control | This is the compile-time contract Phase 54 must describe truthfully. |

### Supporting

| Library / Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Avalonia.Themes.Fluent | 11.3.15 | Theme package for sample hosts | Keep for sample host builds; not a Phase 54 change driver. |
| xUnit | 2.5.3 | Existing unsupported/runtime contract tests | Reuse for proof references; do not add a new framework. |
| Microsoft.NET.Test.Sdk | 17.8.0 | Existing test runner | Reuse in validation lanes. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Updating `README.md` + `samples/avalonia-mdcad-control/QUICKSTART.md` | New standalone onboarding doc | Would leave the most visible stale docs in place. |
| Reusing Phase 53 proof surfaces | New docs-specific proof harness | Unnecessary scope; proof surfaces already exist. |
| Minimal sample path cleanup | Leaving the personal path untouched | Keeps a misleading onboarding example in repo code/docs. |

**Installation:** No new dependencies are required for Phase 54.

**Version verification:**
- `dotnet --version` → `10.0.300`
- `dotnet list .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj package` → Avalonia packages resolved to `11.3.15`
- `dotnet list .\samples\avalonia-host\AvaloniaHost.csproj package` → Avalonia packages resolved to `11.3.15`
- `dotnet list .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj package` → xUnit `2.5.3`, `Microsoft.NET.Test.Sdk` `17.8.0`

## Architecture Patterns

### Recommended Project Structure

```text
README.md                                    # Top-level support contract and quick entry points
samples/
├── avalonia-mdcad-control/
│   └── QUICKSTART.md                        # Consumer wiring guide for the reusable control
├── avalonia-host-minimal/
│   ├── AvaloniaHostMinimal.csproj           # Plain net10 compile/build proof surface
│   └── ViewModels/MainWindowViewModel.cs    # Minimal sample JSONL onboarding input
└── avalonia-host/
    ├── AvaloniaHost.csproj                  # Windows runtime proof surface
    └── MainWindow.axaml.cs                  # Host-owned runtime/proof status lines
.planning/phases/54-docs-and-onboarding-truthfulness/
└── 54-VALIDATION.md                         # Phase 54 truth-check ledger
```

### Pattern 1: Separate proof surfaces by contract
**What:** Keep compile/build proof on `samples/avalonia-host-minimal` and runtime proof on `samples/avalonia-host`.
**When to use:** Every time docs describe what v1.9 “supports.”
**Example:**
```xml
<!-- Source: samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj -->
<TargetFramework>net10.0</TargetFramework>
<ProjectReference Include="..\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj" />
```

### Pattern 2: Source unsupported-platform docs from the canonical runtime message
**What:** Reuse the exact unsupported-platform meaning already encoded in the control/runtime boundary.
**When to use:** README, QUICKSTART, and any onboarding/proof note that describes non-Windows behavior.
**Example:**
```csharp
// Source: samples/avalonia-mdcad-control/Host/MdCadUnsupportedRuntime.cs
internal const string UnsupportedRuntimeMessage =
    "This host/control is valid, but embedded mdCAD viewing is Windows-only and will not launch on the current platform.";
```

### Pattern 3: Treat JSONL path examples as replace-me onboarding data, not proof of runtime support
**What:** Sample JSONL paths must be obviously local/replaceable or omitted.
**When to use:** Quickstart snippets and minimal sample defaults.
**Example:**
```csharp
// Source: samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs
public string JsonlPath { get; } =
    @"C:\Users\RodionRadchenko\source\repos\GeoMate\artifacts\repl\geo-mate-model.jsonl";
```

### Anti-Patterns to Avoid

- **Blurring build proof with runtime proof:** Do not describe `dotnet run` on the minimal host as the proof of v1.9 support.
- **Calling the control itself Windows-only:** The control/reference contract is plain `net10.0`; only the embedded viewer launch path is Windows-only.
- **Inventing new unsupported wording:** Use the canonical implemented behavior instead.
- **Mutating old proof artifacts to tell a new docs story:** Add Phase 54 validation/proof references instead.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Compile-time compatibility proof | New throwaway proof host | `samples/avalonia-host-minimal` | Already locked as the Phase 53 compile/build proof surface. |
| Windows runtime proof | New UI automation or alternate host | `samples/avalonia-host` + existing tests + Phase 53 checklist | Real child-HWND lifecycle proof is already established there. |
| Unsupported-platform copy | New marketing phrasing disconnected from code | `MdCadUnsupportedRuntime.UnsupportedRuntimeMessage` + existing tests | Prevents docs drift from actual behavior. |
| Docs validation | Ad hoc manual eyeballing only | Phase 54 validation rows plus targeted grep/build/test commands | Keeps support-boundary wording auditable. |

**Key insight:** Phase 54 should document and lock the boundary that already exists; it should not create a parallel proof system.

## Common Pitfalls

### Pitfall 1: Reverting to pre-Phase-52 Windows TFM wording
**What goes wrong:** Docs tell consumers they must target `net10.0-windows10.0.19041.0`.
**Why it happens:** README and QUICKSTART still reflect the old v1.8/v1.9-pre-Phase-52 story.
**How to avoid:** Use the csproj files as source-of-truth anchors before editing docs.
**Warning signs:** Any new doc line that says the control or minimal host is Windows-targeted.

### Pitfall 2: Overpromising runtime support from the minimal sample
**What goes wrong:** Readers infer that because the minimal host builds on plain `net10.0`, the embedded viewer is cross-platform at runtime.
**Why it happens:** Build and run steps are presented together without a contract split.
**How to avoid:** Label the minimal sample as compile/build proof and the diagnostic host as Windows runtime proof.
**Warning signs:** README sections that use “works on net10.0” without qualifying compile-time vs runtime.

### Pitfall 3: Leaving unsupported-platform behavior undocumented
**What goes wrong:** Consumers only discover the Windows-only runtime boundary by running the control.
**Why it happens:** The implementation/test truth lives in code, not in onboarding docs.
**How to avoid:** Explicitly document warning/blocked-launch behavior in README and QUICKSTART.
**Warning signs:** Docs mention Windows-only runtime but not what the control actually does on non-Windows.

### Pitfall 4: Treating a local JSONL path as a reusable quickstart example
**What goes wrong:** A copied example fails immediately or looks repo-internal.
**Why it happens:** The current minimal sample and QUICKSTART both use a personal local path.
**How to avoid:** Replace it with an obvious placeholder or no-file default.
**Warning signs:** `C:\Users\...` paths in onboarding snippets.

## Code Examples

Verified patterns from repo sources:

### Plain `net10.0` compile/build proof host
```xml
<!-- Source: samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj -->
<TargetFramework>net10.0</TargetFramework>

<ItemGroup>
  <ProjectReference Include="..\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj" />
</ItemGroup>
```

### Windows runtime proof host remains Windows-targeted
```xml
<!-- Source: samples/avalonia-host/AvaloniaHost.csproj -->
<TargetFramework>net10.0-windows10.0.19041.0</TargetFramework>

<ItemGroup>
  <Content Include="resources\examples\**\*">
    <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
  </Content>
</ItemGroup>
```

### Unsupported-platform status stays explicit
```csharp
// Source: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs
if (IsStartBlocked())
{
    SetLaunchStatus(_isVisualAttached ? "unsupported" : "idle");
    SetAttachStatus(_isVisualAttached ? "unsupported" : "idle");
    UpdateUnsupportedPreLaunchStatus(snapshot);
    return;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Control docs said hosts must target `net10.0-windows10.0.19041.0` | Control project now targets plain `net10.0` | Phase 52 | Docs must stop describing compile-time compatibility as Windows-only. |
| Minimal host implicitly looked like another Windows runtime sample | Minimal host is the locked compile/build proof surface | Phase 53 | README/QUICKSTART must not use it as the runtime proof story. |
| Unsupported runtime truth was mostly implementation/test-only | Unsupported behavior is already explicit in control warning/status logic | Phase 51 | Phase 54 should expose that truth in onboarding docs. |

**Deprecated/outdated:**
- Any README/QUICKSTART wording that says the reusable control itself “stays Windows-only”
- Any doc that says both sample hosts target `net10.0-windows10.0.19041.0`
- Any onboarding snippet that treats the current `GeoMate` JSONL path as reusable consumer guidance

## Open Questions

1. **Does Phase 54 need a sample-code edit, or are doc edits enough?**
   - What we know: the minimal sample and QUICKSTART both expose a personal absolute JSONL path.
   - What's unclear: whether the planner should keep that code unchanged and only annotate it, or normalize the sample itself.
   - Recommendation: treat `samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs` as in scope for the smallest truthful cleanup.

2. **Should Phase 54 create a new onboarding/proof checklist?**
   - What we know: Phase 53 already has the authoritative runtime checklist.
   - What's unclear: whether README + QUICKSTART + `54-VALIDATION.md` are sufficient, or whether a short Phase 54-local proof note improves auditability.
   - Recommendation: prefer `54-VALIDATION.md` plus explicit references to Phase 53 artifacts; only add a new checklist if planning needs a dedicated final sign-off artifact.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| .NET SDK | Build/test proof surfaces and package inspection | ✓ | 10.0.300 | — |
| git | Grep-style doc validation commands | ✓ | 2.51.1.windows.1 | PowerShell `Select-String` |
| PowerShell | Validation scripting on this host | ✓ | 7.5.6 | — |

**Missing dependencies with no fallback:**
- None

**Missing dependencies with fallback:**
- None

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Markdown/static doc checks + existing `dotnet build` smoke lanes + existing xUnit 2.5.3 proof tests |
| Config file | none — define exact commands in `54-VALIDATION.md` |
| Quick run command | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` |
| Full suite command | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~UnsupportedMdCadEmbedBackendTests|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests|FullyQualifiedName~RuntimeRefresh"` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PROOF-02 | README and QUICKSTART state plain `net10.0` compile/build compatibility truthfully | docs static check | `git --no-pager grep -n -E "plain \`net10\.0\`|compile-time|Windows-only" -- README.md samples/avalonia-mdcad-control/QUICKSTART.md` | ✅ |
| PROOF-02 | Plain-host proof remains build-only and green | build smoke | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` | ✅ |
| PROOF-02 | Unsupported-platform docs match implemented behavior | docs + unit | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~UnsupportedMdCadEmbedBackendTests"` | ✅ |
| PROOF-02 | Windows runtime proof stays tied to the diagnostic host, not the minimal host | integration + docs reference | `dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests|FullyQualifiedName~RuntimeRefresh"` | ✅ |

### Sampling Rate

- **Per task commit:** run the smallest impacted command above
- **Per wave merge:** run the full suite command
- **Phase gate:** full suite green plus README/QUICKSTART wording review before `/gsd-verify-work`

### Wave 0 Gaps

- [ ] Add exact Phase 54 doc-static-check command(s) to `54-VALIDATION.md`; current repo has no phase-local docs truth gate yet.
- [ ] Decide whether the onboarding sign-off is satisfied by README/QUICKSTART + validation, or whether a short 54-local proof note is needed.

## Sources

### Primary (HIGH confidence)
- Local repo source: `README.md` — current public support wording and sample-host quickstart sections
- Local repo source: `samples/avalonia-mdcad-control/QUICKSTART.md` — current consumer wiring guide and stale host-TFM guidance
- Local repo source: `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` — actual control TFM is `net10.0`
- Local repo source: `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj` — actual compile/build proof host TFM is `net10.0`
- Local repo source: `samples/avalonia-host/AvaloniaHost.csproj` — actual Windows runtime proof host TFM is `net10.0-windows10.0.19041.0`
- Local repo source: `samples/avalonia-mdcad-control/Host/MdCadUnsupportedRuntime.cs` — canonical unsupported-platform message
- Local repo source: `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` — unsupported and post-launch status behavior
- Local repo source: `samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs` and `UnsupportedMdCadEmbedBackendTests.cs` — verified unsupported-platform assertions
- Local repo source: `.planning/phases/53-consumer-proof-and-windows-regression-closure/53-VALIDATION.md` and `53-MANUAL-CHECKLIST.md` — locked compile/runtime proof split
- Local commands run during research:
  - `dotnet --version`
  - `dotnet list .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj package`
  - `dotnet list .\samples\avalonia-host\AvaloniaHost.csproj package`
  - `dotnet list .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj package`
  - `git --no-pager grep ...` targeted repo audits

### Secondary (MEDIUM confidence)
- None

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - based on local project files and locally resolved package versions
- Architecture: HIGH - based on completed Phase 52/53 artifacts and current sample/test structure
- Pitfalls: HIGH - directly derived from current stale docs vs actual repo behavior

**Research date:** 2026-05-18
**Valid until:** 2026-06-17

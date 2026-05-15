# Quick Task 260515-nvk: Retarget Avalonia control and sample hosts to net10

## Goal

Retarget the Windows-only Avalonia control, sample hosts, and tests from `net8.0-windows10.0.19041.0` to `net10.0-windows10.0.19041.0` so the reusable control can be consumed cleanly from net10 Windows Avalonia hosts without changing the existing Windows-only embedding boundary.

## Task 1

- **files:** `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj`, `samples/avalonia-host/AvaloniaHost.csproj`, `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj`, `samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj`
- **action:** Update the Avalonia control, both in-repo sample hosts, and the control test project to target `net10.0-windows10.0.19041.0` while keeping the current Avalonia package line and the Windows-only runtime/manifest setup unchanged.
- **verify:** `dotnet build samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet build samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet test samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release`
- **done:** All four Avalonia projects build/test on the net10 Windows TFM and continue to reference the same control/runtime seam.

## Task 2

- **files:** `samples/avalonia-mdcad-control/QUICKSTART.md`, `README.md`
- **action:** Update the public consumer guidance to state that the reusable control remains Windows-only and should be referenced from a `net10.0-windows10.0.19041.0` Avalonia host.
- **verify:** `rg "net10\.0-windows10\.0\.19041\.0|Windows-only|Windows only" README.md samples/avalonia-mdcad-control/QUICKSTART.md`
- **done:** The in-repo docs reflect the net10 Windows host requirement instead of leaving the TFM expectation implicit.

# Quick Task 260515-nvk Summary

## Description

Retargeted the reusable Avalonia control, both in-repo host samples, and the control tests to `net10.0-windows10.0.19041.0`, and updated the public guidance so external consumers know the control remains Windows-only.

## What changed

- `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj`
  - Retargeted the reusable control library from `net8.0-windows10.0.19041.0` to `net10.0-windows10.0.19041.0`.
- `samples/avalonia-host/AvaloniaHost.csproj`
  - Retargeted the diagnostic/sample harness to the matching net10 Windows TFM.
- `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj`
  - Retargeted the minimal sealed host sample to the matching net10 Windows TFM.
- `samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj`
  - Retargeted the control test project to the matching net10 Windows TFM.
- `samples/avalonia-mdcad-control/QUICKSTART.md`
  - Added an explicit consumer requirement that host apps target `net10.0-windows10.0.19041.0`.
  - Kept the Windows-only embedding boundary explicit so consumers do not treat the control as a generic cross-platform Avalonia package.
- `README.md`
  - Updated the Windows embedding feature/sample copy to reflect the net10 Windows TFM for the reusable control and sample hosts.

## Verification

- `dotnet build samples\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj -c Release` ✅
- `dotnet build samples\avalonia-host\AvaloniaHost.csproj -c Release` ✅
- `dotnet build samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` ✅
- `dotnet test samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release` ✅

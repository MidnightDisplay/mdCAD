# mdCAD

A lightweight, cross-platform CAD viewer and geometry editor built with C, ECS architecture, and GPU-accelerated rendering; recent work adds a Windows child-HWND embedding bootstrap, startup `--jsonl <absolute-path>` auto-import, explicit `--jsonl-live-refresh` opt-in reuse of the linked observer runtime, and an Avalonia sample host alongside deterministic sketch-solver and observable JSONL workflows.

![image](mdCAD.png)

## Features

- **Geometry primitives**: Points, lines, polylines, arcs, polygons, Bezier curves, helices
- **Point cloud import**: PLY and JSONL geometry log importers with progress tracking
- **Entity-Component-System**: Flecs ECS for scene management with parent-child hierarchies
- **GPU rendering**: Sokol graphics backend (Metal, D3D11, Vulkan, OpenGL, WebGL)
- **Interactive UI**: Dear ImGui interface with scene hierarchy, property inspector, search/filter
- **Editing tools**: Undo/redo, drag-and-drop reparenting, multi-select, translation gizmo
- **Serialization**: Save/load scenes to JSON
- **Windows embedding workflow**: Strict `--embedded --parent-hwnd` child-window launch path plus a reusable Avalonia control that can be referenced from a plain `net10.0` Avalonia host, a minimal sealed host sample that serves as the compile/build proof surface, and a Windows diagnostic harness that remains the runtime proof surface with explicit host-owned status lines and repeatable launch/close proof controls
- **Startup JSONL launch**: Optional `--jsonl <absolute-path>` auto-import plus explicit `--jsonl-live-refresh` opt-in that reuses the existing linked flat refresh behavior without changing the default startup path
- **Cross-platform**: macOS, Windows, Linux, iOS, Android, Web (Emscripten)

## Quick Start

**macOS (Ninja):**
```bash
cmake -B build -G Ninja && ninja -C build
./build/bin/mdCAD
```

**Windows (MSVC + Vulkan):**
```powershell
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON; cmake --build build-vulkan --config Release
.\build-vulkan\bin\Release\mdCAD.exe
```

**Windows startup JSONL launch:**
```powershell
.\build-vulkan\bin\Release\mdCAD.exe --jsonl C:\absolute\path\to\data.jsonl
.\build-vulkan\bin\Release\mdCAD.exe --jsonl C:\absolute\path\to\data.jsonl --jsonl-live-refresh
```

**Windows runtime proof harness:**
```powershell
dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release
dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release
```

`samples/avalonia-host` is the authoritative Windows runtime proof surface. It resolves a bundled JSONL example from `resources/examples`, shows host-owned `launch:`, `attach:`, `jsonl:`, and `live refresh:` status lines, and supports repeated `Launch Session` / `Close Session` cycles from the same window.

**Plain `net10.0` compile/build proof host:**
```powershell
dotnet build samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj -c Release
# Optional local smoke only; this is not proof of cross-platform runtime embedding:
dotnet run --project samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj -c Release
```

`samples/avalonia-host-minimal` proves that a plain `net10.0` Avalonia host can reference `MdCadEmbeddedControl` and build successfully. The embedded mdCAD viewer itself still remains Windows-only at runtime. On unsupported platforms, the host/control is still valid, but embedded viewing will not launch and the control shows the canonical Windows-only warning instead. See `samples/avalonia-mdcad-control/QUICKSTART.md` for step-by-step wiring.

**Web (Emscripten):**
```bash
emcmake cmake -B build-web && cmake --build build-web
open build-web/bin/mdCAD.html
```

See [docs/QUICKSTART.md](docs/QUICKSTART.md) for full build instructions including iOS, Android, and MinGW configurations.

## Platform Support

| Platform | Backend | Status |
|----------|---------|--------|
| macOS | Metal | Supported |
| Windows | D3D11 / Vulkan | Supported |
| Linux | OpenGL | Supported |
| Web | WebGL2 | Experimental |
| iOS | Metal | Experimental |
| Android | GLES3 | Experimental |

## Architecture

mdCAD uses an **Entity-Component-System** (Flecs) for scene management, **Sokol** for cross-platform GPU rendering, and **Dear ImGui** (via cimgui) for the user interface. All modules are header-only C with `static inline` functions.

See [docs/ECS_RENDERING_PRIMER.md](docs/ECS_RENDERING.md) for a detailed architecture overview.

## Third-Party Libraries

| Library | License | URL |
|---------|---------|-----|
| Sokol | zlib/libpng | https://github.com/floooh/sokol |
| Dear ImGui | MIT | https://github.com/ocornut/imgui |
| cimgui | MIT | https://github.com/cimgui/cimgui |
| Flecs | MIT | https://github.com/SanderMertens/flecs |
| cJSON | MIT | https://github.com/DaveGamble/cJSON |

See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for full license texts.

## License

MIT License. See [LICENSE](LICENSE) for details.

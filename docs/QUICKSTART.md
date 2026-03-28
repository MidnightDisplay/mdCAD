# QUICKSTART

## MacOS build using Ninja

Clean, configure and build using Ninja:
``` bash
rm -rf build && cmake -B build -G Ninja && ninja -C build
```

Configure and build:
``` bash
cmake -B build -G Ninja && ninja -C build
```

Quick build & run:
``` bash
ninja -C build -d stats && ./build/bin/mdCAD
```

## Native Math Validation (Phase 2 pilot)

The standalone harness is the primary regression and benchmark surface for this phase. Launching `mdCAD` afterward is an optional manual smoke pass that remains secondary.

### macOS / Ninja

Build the standalone harness:
``` bash
cmake -B build -G Ninja && ninja -C build mdcad_math_harness
```

Run the strict compare suite:
``` bash
cmake -B build -G Ninja && ninja -C build math-regression
```

Run the primitive benchmark suite:
``` bash
cmake -B build -G Ninja && ninja -C build math-bench
```

Run the full native validation workflow:
``` bash
cmake -B build -G Ninja && ninja -C build math-validation
```

Optional manual smoke pass:
``` bash
./build/bin/mdCAD
```

### Windows Vulkan

Configure the Visual Studio Vulkan build:
```powershell
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON
```

Build the standalone harness:
```powershell
cmake --build build-vulkan --config Release --target mdcad_math_harness
```

Run the strict compare suite:
```powershell
cmake --build build-vulkan --config Release --target math-regression
```

Run the primitive benchmark suite:
```powershell
cmake --build build-vulkan --config Release --target math-bench
```

Run the full native validation workflow:
```powershell
cmake --build build-vulkan --config Release --target math-validation
```

## Phase 5 Windows Vulkan hard gate

Run this exact sequence for the HOT-04 hard gate on Windows MSVC Vulkan:
```powershell
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON
cmake --build build-vulkan --config Release --target mdcad_math_harness
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode compare --strict
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode bench --iterations 20000
cmake --build build-vulkan --config Release --target math-validation
.\build-vulkan\bin\Release\mdCAD.exe
```

MinGW is smoke-only for this phase:
```powershell
cmake -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw
.\build-mingw\bin\mdCAD.exe
```

## Phase 5 native performance gate evaluation

Build the harness, capture baseline/candidate artifacts, and write provenance on macOS:
```bash
cmake -B build -G Ninja && ninja -C build mdcad_math_harness
./build/bin/mdcad_math_harness --mode bench --iterations 2000000 > .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/macos-metal/baseline/bench-run1.txt
printf "source_commit=%s\ncapture_cmd=./build/bin/mdcad_math_harness --mode bench --iterations 2000000\n" "$(git rev-parse HEAD)" > .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/macos-metal/baseline/provenance.txt
./build/bin/mdcad_math_harness --mode bench --iterations 2000000 > .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/macos-metal/candidate/bench-run1.txt
printf "source_commit=%s\ncapture_cmd=./build/bin/mdcad_math_harness --mode bench --iterations 2000000\n" "$(git rev-parse HEAD)" > .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/macos-metal/candidate/provenance.txt
```

Capture baseline/candidate artifacts and provenance on Windows MSVC Vulkan:
```powershell
cmake --build build-vulkan --config Release --target mdcad_math_harness
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode bench --iterations 2000000 > .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/baseline/bench-run1.txt
printf "source_commit=%s\ncapture_cmd=.\\build-vulkan\\bin\\Release\\mdcad_math_harness.exe --mode bench --iterations 2000000\n" "$(git rev-parse HEAD)" > .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/baseline/provenance.txt
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode bench --iterations 2000000 > .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/candidate/bench-run1.txt
printf "source_commit=%s\ncapture_cmd=.\\build-vulkan\\bin\\Release\\mdcad_math_harness.exe --mode bench --iterations 2000000\n" "$(git rev-parse HEAD)" > .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/candidate/provenance.txt
```

Evaluate both targets:
```bash
python3 scripts/eval_math_bench.py \
  --label macos-metal \
  --baseline .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/macos-metal/baseline/bench-run1.txt \
  --candidate .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/macos-metal/candidate/bench-run1.txt \
  --output .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/macos-metal/candidate/bench-eval.md

python3 scripts/eval_math_bench.py \
  --label windows-vulkan-msvc \
  --baseline .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/baseline/bench-run1.txt \
  --candidate .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/candidate/bench-run1.txt \
  --output .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/candidate/bench-eval.md
```

Use high-iteration captures (`--iterations 2000000`) for Phase 5 perf evidence to reduce timer-noise outliers in short-running bench cases. If a case is marginal (`5.0% < slowdown <= 8.0%`), capture one rerun (`bench-run2.txt`) and re-run evaluator with `--rerun`.

## Phase 3 macOS parity smoke

The harness-driven validation remains the primary automated gate for this phase. Launching `mdCAD` afterward is a manual smoke step to confirm the migrated camera and transform paths still look stable in the live viewport.

Run the Phase 3 macOS workflow in this order:
``` bash
cmake -B build -G Ninja && ninja -C build math-validation
./build/bin/mdcad_math_harness --mode compare --strict
./build/bin/mdCAD
```

Manual smoke checklist:

- orbit with left-drag
- pan with shift+left or middle-drag
- zoom with the wheel
- confirm visible geometry remains stable while moving the camera
- if parented entities are present in the current scene, confirm child geometry continues following the parent without visible drift

## Phase 4 interaction parity smoke

Run the Phase 4 interaction workflow in this order:
``` bash
cmake -B build -G Ninja && ninja -C build math-validation
./build/bin/mdcad_math_harness --mode compare --strict
./build/bin/mdcad_math_harness --mode bench
./build/bin/mdCAD
```

Manual smoke checklist:

- hover and click around thin lines/points to confirm pick parity
- drag each gizmo axis and plane handle to confirm stable interaction with no start jump
- in geometry mode, drag selected vertices on transformed entities and confirm local-space edits remain correct
- run undo/redo after drag operations and confirm positions/vertices restore exactly

## Windows build (MinGW or MSVC)

### MinGW Vulkan Build (Default MinGW)

Optional clean:
```powershell
Remove-Item -Recurse -Force build-mingw
```

Configure, build & run:
```powershell
# MinGW always uses Vulkan backend
cmake -B build-mingw -G "MinGW Makefiles"; cmake --build build-mingw; .\build-mingw\bin\mdCAD.exe
```

### Visual Studio 2026 with Vulkan (Default MSVC)

Optional clean:
```powershell
Remove-Item -Recurse -Force build-vulkan
```

Configure, build & run:
```powershell
# MSVC with Vulkan backend - requires Vulkan SDK installed
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON; cmake --build build-vulkan --config Release; .\build-vulkan\bin\Release\mdCAD.exe
```

Optionally, open the .slnx solution with Visual Studio and build+run mdCAD project using F5

### Visual Studio 2026 with D3D11 (Optional MSVC)
Optional clean:
```powershell
Remove-Item -Recurse -Force build-msvc
```

Configure, build & run:
```powershell
# MSVC default is D3D11
cmake -B build-msvc -G "Visual Studio 18"; cmake --build build-msvc --config Release; .\build-msvc\bin\Release\mdCAD.exe
```

Optionally, open the .slnx solution with Visual Studio and build+run mdCAD project using F5

## Phase 6 serializer workflow (converter + targeted checks)

Use this when validating the serializer/save-load long-tail migration with the light gate.

### 1) Compile gate (Windows Vulkan)

```powershell
cmake --build build-vulkan --config Release --target mdcad_math_harness
```

### 2) Convert old scene files to schema v2

```powershell
python scripts/scene_format_convert.py --input <legacy-scene.json> --output <converted-scene.json>
```

### 3) Run targeted serializer validation checklist

Checklist artifact:

```text
.planning/phases/06-serializer-and-save-load-long-tail-migration/evidence/serializer-roundtrip-checklist.md
```

Required targeted fields to record:

- entity count
- parent links
- transform fields (`position`, `rotation`, `scale`)
- geometry type preservation

## Phase 7 importer workflow

Use this when validating the importer long-tail migration (JSONL/PLY math-path cutline).

### 1) Compile gate (Windows Vulkan)

```powershell
cmake --build build-vulkan --config Release --target mdcad_math_harness
```

### 2) Full gate (wave/phase-level)

```powershell
cmake --build build-vulkan --config Release --target math-validation
```

### 3) Run targeted importer checklist and report

Checklist artifact:

```text
.planning/phases/07-import-pipeline-long-tail-migration/evidence/importer-targeted-checklist.md
```

Report artifact:

```text
.planning/phases/07-import-pipeline-long-tail-migration/evidence/importer-targeted-check-report.md
```

Required targeted fields to record for representative and variant/converted samples:

- placement
- orientation
- scale
- entity count
- triangle count (when applicable)
- parenting structure

Correctness-delta recording requirements:

- `Correctness Delta vs Previous Import Behavior` must be explicitly filled
- every visible delta must include an `Implementation Analysis Reference`
- every visible delta must include a `Mathematical Justification`

Chunk/progress safety rule:

- treat any `Chunked Import / Progress Semantics` regression as a **blocker**

## Phase 8 undo/editor workflow

Use this when validating undo/editor utility migration and Phase 8 glue burn-down.

### 1) Compile gate (Windows Vulkan)

```powershell
cmake --build build-vulkan --config Release --target mdcad_math_harness
```

### 2) Full gate (wave/phase-level)

```powershell
cmake --build build-vulkan --config Release --target math-validation
```

### 3) Run targeted undo/editor checklist and report

Checklist artifact:

```text
.planning/phases/08-undo-editor-utility-migration-and-glue-burn-down/evidence/undo-editor-targeted-checklist.md
```

Report artifact:

```text
.planning/phases/08-undo-editor-utility-migration-and-glue-burn-down/evidence/undo-editor-targeted-check-report.md
```

Glue inventory artifact:

```text
.planning/phases/08-undo-editor-utility-migration-and-glue-burn-down/evidence/glue-inventory.md
```

Mandatory workflow coverage:

- undo/redo transform edits
- gizmo vertex edit workflow
- inspector edit workflow

## Phase 9 long-tail closure workflow (manual + perf + boundary)

Use this for final v1.1 closure evidence across `VAL-03` and `TRED-02`, with `VAL-02` perf status explicitly referenced.

### 1) Wave/phase automated gate

```powershell
cmake --build build-vulkan --config Release --target math-validation
```

### 2) Review required evidence artifacts

- Manual checklist:
  - `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-checklist.md`
- Manual report:
  - `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-report.md`
- Perf evaluations:
  - `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/candidate/bench-eval.md`
  - `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/bench-eval.md`
- Boundary finalization:
  - `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/boundary/thin-entrypoint-boundary-finalization.md`

### 3) Execute integrated manual long-tail smoke pass

Run `mdCAD` on required native targets and execute serializer -> import -> undo -> editor workflows using the checklist:

```text
.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-checklist.md
```

Record all outcomes in:

```text
.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-report.md
```

### Blocking policy (D-03)

- Treat any unresolved `FAIL` or `BLOCKED` status in required compare/perf/manual gates as blocking for Phase 9 completion.
- Do not convert blocked/manual-unexecuted checks into synthetic pass results.

## iOS build using Xcode

**NOTE** No conventional file system, loaders and savers will not work. Hotkeys untested, should work, requires a keyboard. 

Clean, configure and generate Xcode project:
``` bash
rm -rf build-ios && cmake -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
```

Configure only (if build-ios doesn't exist):
``` bash
cmake -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
```

Open in Xcode:
``` bash
open build-ios/mdCAD.xcodeproj
```

Build from command line for simulator:
``` bash
xcodebuild -project build-ios/mdCAD.xcodeproj -scheme mdCAD -sdk iphonesimulator -configuration Release
```

Build from command line for device (requires code signing):
``` bash
xcodebuild -project build-ios/mdCAD.xcodeproj -scheme mdCAD -sdk iphoneos -configuration Release
```

**Note:** For device builds, you'll need to configure code signing in Xcode (open the project, select your team in Signing & Capabilities).

## Android build (No conventional desktop file system, loaders and savers will not work)

**NOTE** No conventional file system, loaders and savers will not work. Hotkeys untested, should work, requires a keyboard. (same as iOS)

```bash
# GLES3 backend, requires Android SDK/NDK
cd android && ./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
# Debug logs (for investigating known issues - see CHECKPOINT.md):
# adb logcat -s imgui_storage:* alpha_polylines:*
```

## Web build using emscripten (experimental)

Clean, configure and build using Emscripten:
``` bash 
rm -rf build && emcmake cmake -B build-web && cmake --build build-web
```

Configure and build:
``` bash 
emcmake cmake -B build-web && cmake --build build-web
```

Build:
``` bash
cmake --build build-web
```

### Running the web build

Open directly in browser (works because `-sSINGLE_FILE` is set):
``` bash
# macOS
open build-web/bin/mdCAD.html

# Linux
xdg-open build-web/bin/mdCAD.html

# Windows
start build-web/bin/mdCAD.html
```

Or serve with a local web server:
``` bash
python3 -m http.server 8000 -d build-web/bin
# Then open http://localhost:8000/mdCAD.html
```

### Puppeteer testing

Automated browser testing with Puppeteer for debugging and UI interaction testing.

#### Setup

Install dependencies (from project root):
``` bash
cd scripts && npm install
```

#### Debug script

Captures console output and screenshots from the WASM app:
``` bash
# Basic run (headless, 5 second runtime)
node scripts/debug-wasm.mjs

# With screenshot
node scripts/debug-wasm.mjs --screenshot

# Visible browser window
node scripts/debug-wasm.mjs --visible

# Custom HTML path and runtime (ms)
node scripts/debug-wasm.mjs build-web/bin/mdCAD.html 10000

# All options
node scripts/debug-wasm.mjs build-web/bin/mdCAD.html 5000 --visible --screenshot
```

#### ImGui interaction test

Demonstrates automated UI interaction with ImGui sliders:
``` bash
# Headless
node scripts/test-imgui.mjs

# Watch the browser
node scripts/test-imgui.mjs --visible
```

#### Output

Screenshots and test artifacts are saved to `scripts/test-output/` (gitignored).

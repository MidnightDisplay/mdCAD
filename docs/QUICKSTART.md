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

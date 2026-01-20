# QUICKSTART

## Native build using Ninja

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
ninja -C build -d stats && ./build/bin/skl_tmp
```

## iOS build using Xcode

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
open build-ios/skl_tmp.xcodeproj
```

Build from command line for simulator:
``` bash
xcodebuild -project build-ios/skl_tmp.xcodeproj -scheme skl_tmp -sdk iphonesimulator -configuration Release
```

Build from command line for device (requires code signing):
``` bash
xcodebuild -project build-ios/skl_tmp.xcodeproj -scheme skl_tmp -sdk iphoneos -configuration Release
```

**Note:** For device builds, you'll need to configure code signing in Xcode (open the project, select your team in Signing & Capabilities).

## Web build using emscripten

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
open build-web/bin/skl_tmp.html

# Linux
xdg-open build-web/bin/skl_tmp.html

# Windows
start build-web/bin/skl_tmp.html
```

Or serve with a local web server:
``` bash
python3 -m http.server 8000 -d build-web/bin
# Then open http://localhost:8000/skl_tmp.html
```

## Puppeteer testing

Automated browser testing with Puppeteer for debugging and UI interaction testing.

### Setup

Install dependencies (from project root):
``` bash
cd scripts && npm install
```

### Debug script

Captures console output and screenshots from the WASM app:
``` bash
# Basic run (headless, 5 second runtime)
node scripts/debug-wasm.mjs

# With screenshot
node scripts/debug-wasm.mjs --screenshot

# Visible browser window
node scripts/debug-wasm.mjs --visible

# Custom HTML path and runtime (ms)
node scripts/debug-wasm.mjs build-web/bin/skl_tmp.html 10000

# All options
node scripts/debug-wasm.mjs build-web/bin/skl_tmp.html 5000 --visible --screenshot
```

### ImGui interaction test

Demonstrates automated UI interaction with ImGui sliders:
``` bash
# Headless
node scripts/test-imgui.mjs

# Watch the browser
node scripts/test-imgui.mjs --visible
```

### Output

Screenshots and test artifacts are saved to `scripts/test-output/` (gitignored).
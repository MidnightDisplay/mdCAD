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
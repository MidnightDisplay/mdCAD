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
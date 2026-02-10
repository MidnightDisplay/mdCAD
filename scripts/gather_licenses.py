#!/usr/bin/env python3
"""
gather_licenses.py — Scan vendors/ and build/_deps/ for LICENSE files
and generate THIRD_PARTY_LICENSES.md at the project root.

Falls back to hardcoded license data when files aren't found (common when
vendors are vendored as single-file amalgamations without LICENSE files).

Usage:
    python3 scripts/gather_licenses.py
"""

import os
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
OUTPUT_FILE = PROJECT_ROOT / "THIRD_PARTY_LICENSES.md"

# Known libraries with hardcoded fallback data.
# Each entry: (name, url, license_type, copyright, full_text)
# search_dirs: list of relative dirs under PROJECT_ROOT to look for LICENSE/COPYING
LIBRARIES = [
    {
        "name": "Sokol",
        "url": "https://github.com/floooh/sokol",
        "license_type": "zlib/libpng",
        "copyright": "(c) 2018 Andre Weissflog",
        "search_dirs": ["vendors/libsokol", "build/_deps/libsokol-src"],
        "fallback_text": """\
zlib/libpng license

Copyright (c) 2018 Andre Weissflog

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the
use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software in a
    product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not
    be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.""",
    },
    {
        "name": "Dear ImGui",
        "url": "https://github.com/ocornut/imgui",
        "license_type": "MIT",
        "copyright": "(c) 2014-2024 Omar Cornut",
        "search_dirs": [],  # Bundled inside cimgui
        "fallback_text": """\
The MIT License (MIT)

Copyright (c) 2014-2024 Omar Cornut

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.""",
    },
    {
        "name": "cimgui",
        "url": "https://github.com/cimgui/cimgui",
        "license_type": "MIT",
        "copyright": "(c) 2015 Stephan Dilly",
        "search_dirs": ["vendors/libcimgui", "build/_deps/libcimgui-src"],
        "fallback_text": """\
The MIT License (MIT)

Copyright (c) 2015 Stephan Dilly

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.""",
    },
    {
        "name": "Flecs",
        "url": "https://github.com/SanderMertens/flecs",
        "license_type": "MIT",
        "copyright": "(c) 2019 Sander Mertens",
        "search_dirs": ["vendors/flecs", "build/_deps/flecs-src"],
        "fallback_text": """\
The MIT License (MIT)

Copyright (c) 2019 Sander Mertens

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.""",
    },
    {
        "name": "cJSON",
        "url": "https://github.com/DaveGamble/cJSON",
        "license_type": "MIT",
        "copyright": "(c) 2009-2017 Dave Gamble and cJSON contributors",
        "search_dirs": ["vendors/cjson"],
        "fallback_text": """\
The MIT License (MIT)

Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.""",
    },
]


def find_license_file(search_dirs):
    """Search directories for LICENSE or COPYING files."""
    license_names = ["LICENSE", "LICENSE.txt", "LICENSE.md", "COPYING", "COPYING.txt"]
    for rel_dir in search_dirs:
        d = PROJECT_ROOT / rel_dir
        if not d.is_dir():
            continue
        for name in license_names:
            p = d / name
            if p.is_file():
                return p.read_text(encoding="utf-8").strip()
    return None


def generate():
    lines = ["# Third-Party Licenses", "", "mdCAD uses the following open-source libraries:", ""]

    for lib in LIBRARIES:
        text = find_license_file(lib["search_dirs"])
        source = "file"
        if text is None:
            text = lib["fallback_text"]
            source = "fallback"

        lines.append("---")
        lines.append("")
        lines.append(f"## {lib['name']}")
        lines.append("")
        lines.append(f"- **URL:** {lib['url']}")
        lines.append(f"- **License:** {lib['license_type']}")
        lines.append(f"- **Copyright:** {lib['copyright']}")
        lines.append("")
        lines.append("```")
        lines.append(text)
        lines.append("```")
        lines.append("")

        print(f"  {lib['name']:20s}  {lib['license_type']:15s}  ({source})")

    content = "\n".join(lines)
    OUTPUT_FILE.write_text(content, encoding="utf-8")
    print(f"\nWrote {OUTPUT_FILE}")


if __name__ == "__main__":
    print("Gathering third-party licenses...")
    generate()

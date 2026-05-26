# Plan: 3D Pixel Text PLY Generator

## Context

We want a flashy screenshot for the project README — colourful 3D text rendered as a point cloud that can be imported into mdCAD. The script generates retro pixel-font text where each "lit" pixel becomes a dense cube/slab of point cloud points, coloured with a rainbow gradient.

**New file:** `scripts/generate_ply_text.py`
**Reference:** `scripts/generate_ply_grid.py` (same PLY format, argparse style, console output)

---

## CLI Interface

```
python scripts/generate_ply_text.py \
  --text "MDCAD"           # text to render (default: "MDCAD")
  --density 10             # points per pixel edge (default: 10)
  --depth-mode cube        # cube | custom (default: cube)
  --depth-layers 3         # layers for custom mode (default: 3)
  --spacing 0.1            # distance between sub-points (default: 0.1)
  --color-mode smooth      # smooth | pixel | character (default: smooth)
  --font-size small        # small (3x5) | large (5x7) (default: small)
  -o text.ply              # output file (default: text.ply)
```

---

## Font Design

Two bitmap fonts stored as `dict[str, list[str]]` using `#` for lit and `.` for unlit.

- **Small (3x5):** 3 cols x 5 rows — minimal retro LED look
- **Large (5x7):** 5 cols x 7 rows — classic LED matrix, more readable

Character set: A-Z, 0-9, space, `.` `,` `-` `!` `?` `:` `/` (41 chars)

Characters laid out horizontally with 1-pixel gap between them.

---

## Point Generation

For each lit pixel at grid position `(px_x, px_y)`:

```
depth_points = density           (cube mode)
             = depth_layers      (custom mode)

for dz in range(depth_points):
    for dy in range(density):
        for dx in range(density):
            x = (px_x * density + dx) * spacing
            y = (px_y * density + dy) * spacing
            z = dz * spacing
```

Y is flipped so row 0 (top of bitmap) maps to highest Y — text reads right-side-up.

Adjacent pixels' sub-grids are seamlessly contiguous (no gaps, no overlaps).

---

## Color Modes (Rainbow HSV 0-360)

| Mode | Hue assignment |
|------|----------------|
| `smooth` | Continuous across all points by world X position |
| `pixel` | Discrete per pixel-column along text direction |
| `character` | Discrete per character index |

All use full saturation/value (S=1, V=1). Single-char edge case guarded with `max(n-1, 1)`.

Uses `colorsys.hsv_to_rgb` from stdlib — no external dependencies.

---

## Output

ASCII PLY matching `ply_loader.h` expectations:
```
ply
format ascii 1.0
element vertex {N}
property float x
property float y
property float z
property uchar red
property uchar green
property uchar blue
end_header
```

Console output matches `generate_ply_grid.py` style: text, font, density, depth, lit pixels, total points, file size, dimensions.

---

## Functions

| Function | Purpose |
|----------|---------|
| `hsv_to_rgb_bytes(h, s, v)` | HSV (h 0-360) to RGB (0-255) tuple |
| `get_char_bitmap(char, font)` | Lookup with fallback to `?` |
| `generate_text_ply(...)` | Main generation: enumerate lit pixels, generate sub-grids, assign colors, write PLY |
| `main()` | Argparse, validation, console output, file size |

---

## Implementation Steps

1. Create file with shebang, docstring, imports (`argparse`, `sys`, `os`, `colorsys`)
2. Define `FONT_SMALL` — all 41 characters (3x5 bitmaps)
3. Define `FONT_LARGE` — all 41 characters (5x7 bitmaps)
4. Implement `hsv_to_rgb_bytes()` and `get_char_bitmap()`
5. Implement `generate_text_ply()` — lit pixel enumeration, sub-grid generation, color assignment, PLY write
6. Implement `main()` with argparse, validation, console output
7. Test: generate a few variants and verify import into mdCAD

---

## Verification

```bash
# Small test — fast, ~3k points
python scripts/generate_ply_text.py --text "HI" --density 5 --depth-mode custom --depth-layers 2 -o /tmp/test_hi.ply

# Medium — cube mode, smooth rainbow
python scripts/generate_ply_text.py --text "MDCAD" --density 10 -o /tmp/test_mdcad.ply

# Large font, per-character coloring
python scripts/generate_ply_text.py --text "HELLO" --font-size large --color-mode character --density 10 -o /tmp/test_hello.ply

# Verify: import each PLY into mdCAD via File > Import PLY Point Cloud
```

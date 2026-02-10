#!/usr/bin/env python3
"""
Generate PLY point cloud files with 3D pixel-font text.

Each lit pixel in the bitmap font becomes a dense cube/slab of points,
colored with a rainbow gradient. Great for flashy screenshots.

Usage:
    python generate_ply_text.py --text "MDCAD" --density 10 -o text.ply
    python generate_ply_text.py --text "HELLO" --font-size large --color-mode character -o hello.ply
"""

import argparse
import colorsys
import os
import sys

# ---------------------------------------------------------------------------
# Bitmap fonts — '#' = lit, '.' = unlit
# ---------------------------------------------------------------------------

# Small font: 3 columns x 5 rows
FONT_SMALL = {
    'A': [
        ".#.",
        "#.#",
        "###",
        "#.#",
        "#.#",
    ],
    'B': [
        "##.",
        "#.#",
        "##.",
        "#.#",
        "##.",
    ],
    'C': [
        ".##",
        "#..",
        "#..",
        "#..",
        ".##",
    ],
    'D': [
        "##.",
        "#.#",
        "#.#",
        "#.#",
        "##.",
    ],
    'E': [
        "###",
        "#..",
        "##.",
        "#..",
        "###",
    ],
    'F': [
        "###",
        "#..",
        "##.",
        "#..",
        "#..",
    ],
    'G': [
        ".##",
        "#..",
        "#.#",
        "#.#",
        ".##",
    ],
    'H': [
        "#.#",
        "#.#",
        "###",
        "#.#",
        "#.#",
    ],
    'I': [
        "###",
        ".#.",
        ".#.",
        ".#.",
        "###",
    ],
    'J': [
        "..#",
        "..#",
        "..#",
        "#.#",
        ".#.",
    ],
    'K': [
        "#.#",
        "#.#",
        "##.",
        "#.#",
        "#.#",
    ],
    'L': [
        "#..",
        "#..",
        "#..",
        "#..",
        "###",
    ],
    'M': [
        "#.#",
        "###",
        "###",
        "#.#",
        "#.#",
    ],
    'N': [
        "#.#",
        "##.",
        "###",
        "#.#",
        "#.#",
    ],
    'O': [
        ".#.",
        "#.#",
        "#.#",
        "#.#",
        ".#.",
    ],
    'P': [
        "##.",
        "#.#",
        "##.",
        "#..",
        "#..",
    ],
    'Q': [
        ".#.",
        "#.#",
        "#.#",
        "##.",
        ".##",
    ],
    'R': [
        "##.",
        "#.#",
        "##.",
        "#.#",
        "#.#",
    ],
    'S': [
        ".##",
        "#..",
        ".#.",
        "..#",
        "##.",
    ],
    'T': [
        "###",
        ".#.",
        ".#.",
        ".#.",
        ".#.",
    ],
    'U': [
        "#.#",
        "#.#",
        "#.#",
        "#.#",
        ".#.",
    ],
    'V': [
        "#.#",
        "#.#",
        "#.#",
        ".#.",
        ".#.",
    ],
    'W': [
        "#.#",
        "#.#",
        "###",
        "###",
        "#.#",
    ],
    'X': [
        "#.#",
        "#.#",
        ".#.",
        "#.#",
        "#.#",
    ],
    'Y': [
        "#.#",
        "#.#",
        ".#.",
        ".#.",
        ".#.",
    ],
    'Z': [
        "###",
        "..#",
        ".#.",
        "#..",
        "###",
    ],
    '0': [
        ".#.",
        "#.#",
        "#.#",
        "#.#",
        ".#.",
    ],
    '1': [
        ".#.",
        "##.",
        ".#.",
        ".#.",
        "###",
    ],
    '2': [
        ".#.",
        "#.#",
        "..#",
        ".#.",
        "###",
    ],
    '3': [
        "###",
        "..#",
        ".#.",
        "..#",
        "###",
    ],
    '4': [
        "#.#",
        "#.#",
        "###",
        "..#",
        "..#",
    ],
    '5': [
        "###",
        "#..",
        "##.",
        "..#",
        "##.",
    ],
    '6': [
        ".##",
        "#..",
        "###",
        "#.#",
        ".#.",
    ],
    '7': [
        "###",
        "..#",
        ".#.",
        ".#.",
        ".#.",
    ],
    '8': [
        ".#.",
        "#.#",
        ".#.",
        "#.#",
        ".#.",
    ],
    '9': [
        ".#.",
        "#.#",
        "###",
        "..#",
        "##.",
    ],
    ' ': [
        "...",
        "...",
        "...",
        "...",
        "...",
    ],
    '.': [
        "...",
        "...",
        "...",
        "...",
        ".#.",
    ],
    ',': [
        "...",
        "...",
        "...",
        ".#.",
        "#..",
    ],
    '-': [
        "...",
        "...",
        "###",
        "...",
        "...",
    ],
    '!': [
        ".#.",
        ".#.",
        ".#.",
        "...",
        ".#.",
    ],
    '?': [
        ".#.",
        "#.#",
        "..#",
        ".#.",
        ".#.",
    ],
    ':': [
        "...",
        ".#.",
        "...",
        ".#.",
        "...",
    ],
    '/': [
        "..#",
        "..#",
        ".#.",
        "#..",
        "#..",
    ],
    # --- lowercase ---
    'a': [
        "...",
        "...",
        ".##",
        "#.#",
        ".##",
    ],
    'b': [
        "#..",
        "#..",
        "##.",
        "#.#",
        "##.",
    ],
    'c': [
        "...",
        "...",
        ".##",
        "#..",
        ".##",
    ],
    'd': [
        "..#",
        "..#",
        ".##",
        "#.#",
        ".##",
    ],
    'e': [
        "...",
        ".#.",
        "#.#",
        "##.",
        ".#.",
    ],
    'f': [
        ".##",
        ".#.",
        "###",
        ".#.",
        ".#.",
    ],
    'g': [
        ".##",
        "#.#",
        ".##",
        "..#",
        ".#.",
    ],
    'h': [
        "#..",
        "#..",
        "##.",
        "#.#",
        "#.#",
    ],
    'i': [
        ".#.",
        "...",
        ".#.",
        ".#.",
        ".#.",
    ],
    'j': [
        "..#",
        "...",
        "..#",
        "..#",
        ".#.",
    ],
    'k': [
        "#..",
        "#.#",
        "##.",
        "#.#",
        "#.#",
    ],
    'l': [
        "##.",
        ".#.",
        ".#.",
        ".#.",
        ".##",
    ],
    'm': [
        "...",
        "...",
        "###",
        "###",
        "#.#",
    ],
    'n': [
        "...",
        "...",
        "##.",
        "#.#",
        "#.#",
    ],
    'o': [
        "...",
        "...",
        ".#.",
        "#.#",
        ".#.",
    ],
    'p': [
        "##.",
        "#.#",
        "##.",
        "#..",
        "#..",
    ],
    'q': [
        ".##",
        "#.#",
        ".##",
        "..#",
        "..#",
    ],
    'r': [
        "...",
        "...",
        ".##",
        "#..",
        "#..",
    ],
    's': [
        "...",
        ".##",
        "#..",
        "..#",
        "##.",
    ],
    't': [
        ".#.",
        "###",
        ".#.",
        ".#.",
        ".##",
    ],
    'u': [
        "...",
        "...",
        "#.#",
        "#.#",
        ".##",
    ],
    'v': [
        "...",
        "...",
        "#.#",
        "#.#",
        ".#.",
    ],
    'w': [
        "...",
        "...",
        "#.#",
        "###",
        "#.#",
    ],
    'x': [
        "...",
        "...",
        "#.#",
        ".#.",
        "#.#",
    ],
    'y': [
        "#.#",
        "#.#",
        ".##",
        "..#",
        ".#.",
    ],
    'z': [
        "...",
        "...",
        "###",
        ".#.",
        "###",
    ],
}

# Large font: 5 columns x 7 rows
FONT_LARGE = {
    'A': [
        ".###.",
        "#...#",
        "#...#",
        "#####",
        "#...#",
        "#...#",
        "#...#",
    ],
    'B': [
        "####.",
        "#...#",
        "#...#",
        "####.",
        "#...#",
        "#...#",
        "####.",
    ],
    'C': [
        ".###.",
        "#...#",
        "#....",
        "#....",
        "#....",
        "#...#",
        ".###.",
    ],
    'D': [
        "####.",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        "####.",
    ],
    'E': [
        "#####",
        "#....",
        "#....",
        "###..",
        "#....",
        "#....",
        "#####",
    ],
    'F': [
        "#####",
        "#....",
        "#....",
        "###..",
        "#....",
        "#....",
        "#....",
    ],
    'G': [
        ".###.",
        "#...#",
        "#....",
        "#.###",
        "#...#",
        "#...#",
        ".###.",
    ],
    'H': [
        "#...#",
        "#...#",
        "#...#",
        "#####",
        "#...#",
        "#...#",
        "#...#",
    ],
    'I': [
        "#####",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
        "#####",
    ],
    'J': [
        "..###",
        "...#.",
        "...#.",
        "...#.",
        "#..#.",
        "#..#.",
        ".##..",
    ],
    'K': [
        "#...#",
        "#..#.",
        "#.#..",
        "##...",
        "#.#..",
        "#..#.",
        "#...#",
    ],
    'L': [
        "#....",
        "#....",
        "#....",
        "#....",
        "#....",
        "#....",
        "#####",
    ],
    'M': [
        "#...#",
        "##.##",
        "#.#.#",
        "#.#.#",
        "#...#",
        "#...#",
        "#...#",
    ],
    'N': [
        "#...#",
        "##..#",
        "#.#.#",
        "#..##",
        "#...#",
        "#...#",
        "#...#",
    ],
    'O': [
        ".###.",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        ".###.",
    ],
    'P': [
        "####.",
        "#...#",
        "#...#",
        "####.",
        "#....",
        "#....",
        "#....",
    ],
    'Q': [
        ".###.",
        "#...#",
        "#...#",
        "#...#",
        "#.#.#",
        "#..#.",
        ".##.#",
    ],
    'R': [
        "####.",
        "#...#",
        "#...#",
        "####.",
        "#.#..",
        "#..#.",
        "#...#",
    ],
    'S': [
        ".###.",
        "#...#",
        "#....",
        ".###.",
        "....#",
        "#...#",
        ".###.",
    ],
    'T': [
        "#####",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
    ],
    'U': [
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        ".###.",
    ],
    'V': [
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        ".#.#.",
        ".#.#.",
        "..#..",
    ],
    'W': [
        "#...#",
        "#...#",
        "#...#",
        "#.#.#",
        "#.#.#",
        "##.##",
        "#...#",
    ],
    'X': [
        "#...#",
        "#...#",
        ".#.#.",
        "..#..",
        ".#.#.",
        "#...#",
        "#...#",
    ],
    'Y': [
        "#...#",
        "#...#",
        ".#.#.",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
    ],
    'Z': [
        "#####",
        "....#",
        "...#.",
        "..#..",
        ".#...",
        "#....",
        "#####",
    ],
    '0': [
        ".###.",
        "#...#",
        "#..##",
        "#.#.#",
        "##..#",
        "#...#",
        ".###.",
    ],
    '1': [
        "..#..",
        ".##..",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
        ".###.",
    ],
    '2': [
        ".###.",
        "#...#",
        "....#",
        "..##.",
        ".#...",
        "#....",
        "#####",
    ],
    '3': [
        "#####",
        "....#",
        "...#.",
        "..##.",
        "....#",
        "#...#",
        ".###.",
    ],
    '4': [
        "#...#",
        "#...#",
        "#...#",
        "#####",
        "....#",
        "....#",
        "....#",
    ],
    '5': [
        "#####",
        "#....",
        "####.",
        "....#",
        "....#",
        "#...#",
        ".###.",
    ],
    '6': [
        ".###.",
        "#....",
        "#....",
        "####.",
        "#...#",
        "#...#",
        ".###.",
    ],
    '7': [
        "#####",
        "....#",
        "...#.",
        "..#..",
        ".#...",
        ".#...",
        ".#...",
    ],
    '8': [
        ".###.",
        "#...#",
        "#...#",
        ".###.",
        "#...#",
        "#...#",
        ".###.",
    ],
    '9': [
        ".###.",
        "#...#",
        "#...#",
        ".####",
        "....#",
        "....#",
        ".###.",
    ],
    ' ': [
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
    ],
    '.': [
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
        ".##..",
        ".##..",
    ],
    ',': [
        ".....",
        ".....",
        ".....",
        ".....",
        "..#..",
        "..#..",
        ".#...",
    ],
    '-': [
        ".....",
        ".....",
        ".....",
        ".###.",
        ".....",
        ".....",
        ".....",
    ],
    '!': [
        "..#..",
        "..#..",
        "..#..",
        "..#..",
        ".....",
        "..#..",
        "..#..",
    ],
    '?': [
        ".###.",
        "#...#",
        "....#",
        "..##.",
        "..#..",
        ".....",
        "..#..",
    ],
    ':': [
        ".....",
        "..#..",
        "..#..",
        ".....",
        "..#..",
        "..#..",
        ".....",
    ],
    '/': [
        "....#",
        "...#.",
        "...#.",
        "..#..",
        ".#...",
        ".#...",
        "#....",
    ],
    # --- lowercase ---
    'a': [
        ".....",
        ".....",
        ".###.",
        "....#",
        ".####",
        "#...#",
        ".####",
    ],
    'b': [
        "#....",
        "#....",
        "####.",
        "#...#",
        "#...#",
        "#...#",
        "####.",
    ],
    'c': [
        ".....",
        ".....",
        ".####",
        "#....",
        "#....",
        "#....",
        ".####",
    ],
    'd': [
        "....#",
        "....#",
        ".####",
        "#...#",
        "#...#",
        "#...#",
        ".####",
    ],
    'e': [
        ".....",
        ".....",
        ".###.",
        "#...#",
        "#####",
        "#....",
        ".###.",
    ],
    'f': [
        "..##.",
        ".#...",
        ".#...",
        "####.",
        ".#...",
        ".#...",
        ".#...",
    ],
    'g': [
        ".....",
        ".....",
        ".####",
        "#...#",
        ".####",
        "....#",
        ".###.",
    ],
    'h': [
        "#....",
        "#....",
        "#.##.",
        "##..#",
        "#...#",
        "#...#",
        "#...#",
    ],
    'i': [
        "..#..",
        ".....",
        ".##..",
        "..#..",
        "..#..",
        "..#..",
        ".###.",
    ],
    'j': [
        "...#.",
        ".....",
        "..##.",
        "...#.",
        "...#.",
        "#..#.",
        ".##..",
    ],
    'k': [
        "#....",
        "#....",
        "#..#.",
        "#.#..",
        "##...",
        "#.#..",
        "#..#.",
    ],
    'l': [
        ".##..",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
        ".###.",
    ],
    'm': [
        ".....",
        ".....",
        "##.#.",
        "#.#.#",
        "#.#.#",
        "#...#",
        "#...#",
    ],
    'n': [
        ".....",
        ".....",
        "#.##.",
        "##..#",
        "#...#",
        "#...#",
        "#...#",
    ],
    'o': [
        ".....",
        ".....",
        ".###.",
        "#...#",
        "#...#",
        "#...#",
        ".###.",
    ],
    'p': [
        ".....",
        ".....",
        "####.",
        "#...#",
        "####.",
        "#....",
        "#....",
    ],
    'q': [
        ".....",
        ".....",
        ".####",
        "#...#",
        ".####",
        "....#",
        "....#",
    ],
    'r': [
        ".....",
        ".....",
        "#.##.",
        "##..#",
        "#....",
        "#....",
        "#....",
    ],
    's': [
        ".....",
        ".....",
        ".####",
        "#....",
        ".###.",
        "....#",
        "####.",
    ],
    't': [
        "..#..",
        "..#..",
        ".###.",
        "..#..",
        "..#..",
        "..#.#",
        "..##.",
    ],
    'u': [
        ".....",
        ".....",
        "#...#",
        "#...#",
        "#...#",
        "#..##",
        ".##.#",
    ],
    'v': [
        ".....",
        ".....",
        "#...#",
        "#...#",
        "#...#",
        ".#.#.",
        "..#..",
    ],
    'w': [
        ".....",
        ".....",
        "#...#",
        "#...#",
        "#.#.#",
        "#.#.#",
        ".#.#.",
    ],
    'x': [
        ".....",
        ".....",
        "#...#",
        ".#.#.",
        "..#..",
        ".#.#.",
        "#...#",
    ],
    'y': [
        ".....",
        ".....",
        "#...#",
        "#...#",
        ".####",
        "....#",
        ".###.",
    ],
    'z': [
        ".....",
        ".....",
        "#####",
        "...#.",
        "..#..",
        ".#...",
        "#####",
    ],
}


def hsv_to_rgb_bytes(h, s, v):
    """Convert HSV (h: 0-360, s: 0-1, v: 0-1) to RGB (0-255) tuple."""
    r, g, b = colorsys.hsv_to_rgb(h / 360.0, s, v)
    return int(r * 255), int(g * 255), int(b * 255)


def get_char_bitmap(char, font):
    """Look up a character bitmap. Tries exact char, then uppercase, then '?'."""
    if char in font:
        return font[char]
    if char.upper() in font:
        return font[char.upper()]
    return font.get('?', font['A'])


def generate_text_ply(text, density, depth_mode, depth_layers, spacing,
                      color_mode, font_size, output_file):
    """Generate a PLY file with 3D pixel-font text."""

    font = FONT_LARGE if font_size == 'large' else FONT_SMALL

    # Determine character width from font
    sample = next(iter(font.values()))
    char_width = len(sample[0])
    char_height = len(sample)
    gap = 1  # 1-pixel gap between characters

    # Collect lit pixels: (px_x, px_y, char_index)
    lit_pixels = []
    cursor_x = 0
    for char_idx, ch in enumerate(text):
        bitmap = get_char_bitmap(ch, font)
        for row_idx, row in enumerate(bitmap):
            for col_idx, cell in enumerate(row):
                if cell == '#':
                    px_x = cursor_x + col_idx
                    px_y = row_idx
                    lit_pixels.append((px_x, px_y, char_idx))
        cursor_x += char_width + gap

    if not lit_pixels:
        print("Warning: No lit pixels — text is empty or all spaces.", file=sys.stderr)

    # Compute X range for smooth coloring
    total_pixel_cols = cursor_x - gap if text else 1

    # Depth points along Z
    depth_points = density if depth_mode == 'cube' else depth_layers

    # Generate points
    points = []
    for px_x, px_y, char_idx in lit_pixels:
        # Flip Y so row 0 (top of bitmap) is highest
        flipped_y = (char_height - 1) - px_y

        # Compute hue for this pixel
        if color_mode == 'smooth':
            # Will be overridden per-point below
            pass
        elif color_mode == 'pixel':
            hue = 360.0 * px_x / max(total_pixel_cols - 1, 1)
        else:  # character
            hue = 360.0 * char_idx / max(len(text) - 1, 1)

        for dz in range(depth_points):
            for dy in range(density):
                for dx in range(density):
                    x = (px_x * density + dx) * spacing
                    y = (flipped_y * density + dy) * spacing
                    z = dz * spacing

                    if color_mode == 'smooth':
                        # Continuous hue by world X position
                        max_x = (total_pixel_cols * density - 1) * spacing
                        hue = 360.0 * x / max(max_x, spacing)

                    r, g, b = hsv_to_rgb_bytes(hue, 1.0, 1.0)
                    points.append((x, y, z, r, g, b))

    total_points = len(points)

    # Write PLY file
    with open(output_file, 'w') as f:
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write(f"element vertex {total_points}\n")
        f.write("property float x\n")
        f.write("property float y\n")
        f.write("property float z\n")
        f.write("property uchar red\n")
        f.write("property uchar green\n")
        f.write("property uchar blue\n")
        f.write("end_header\n")

        for pt in points:
            f.write(f"{pt[0]:.6f} {pt[1]:.6f} {pt[2]:.6f} {pt[3]} {pt[4]} {pt[5]}\n")

    return total_points, len(lit_pixels)


def main():
    parser = argparse.ArgumentParser(
        description='Generate PLY point cloud files with 3D pixel-font text.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Default — "MDCAD" in small font, cube mode, smooth rainbow
  python generate_ply_text.py -o text.ply

  # Quick test
  python generate_ply_text.py --text "HI" --density 5 --depth-mode custom --depth-layers 2 -o hi.ply

  # Large font with per-character coloring
  python generate_ply_text.py --text "HELLO" --font-size large --color-mode character --density 10 -o hello.ply
"""
    )

    parser.add_argument('--text', type=str, default='MDCAD',
                        help='Text to render (default: MDCAD)')
    parser.add_argument('--density', type=int, default=10,
                        help='Points per pixel edge (default: 10)')
    parser.add_argument('--depth-mode', choices=['cube', 'custom'], default='cube',
                        help='Depth mode: cube (density layers) or custom (default: cube)')
    parser.add_argument('--depth-layers', type=int, default=3,
                        help='Number of depth layers in custom mode (default: 3)')
    parser.add_argument('--spacing', type=float, default=0.1,
                        help='Distance between sub-points (default: 0.1)')
    parser.add_argument('--color-mode', choices=['smooth', 'pixel', 'character'],
                        default='smooth',
                        help='Color mode: smooth, pixel, or character (default: smooth)')
    parser.add_argument('--font-size', choices=['small', 'large'], default='small',
                        help='Font size: small (3x5) or large (5x7) (default: small)')
    parser.add_argument('-o', '--output', type=str, default='text.ply',
                        help='Output PLY filename (default: text.ply)')

    args = parser.parse_args()

    # Validate
    if args.density < 1:
        print("Error: Density must be at least 1", file=sys.stderr)
        sys.exit(1)
    if args.depth_layers < 1:
        print("Error: Depth layers must be at least 1", file=sys.stderr)
        sys.exit(1)
    if args.spacing <= 0:
        print("Error: Spacing must be positive", file=sys.stderr)
        sys.exit(1)

    font_label = f"{'large (5x7)' if args.font_size == 'large' else 'small (3x5)'}"
    depth_label = f"cube ({args.density})" if args.depth_mode == 'cube' else f"custom ({args.depth_layers})"

    print(f"Generating 3D text: \"{args.text}\"")
    print(f"  Font: {font_label}")
    print(f"  Density: {args.density} points/edge")
    print(f"  Depth: {depth_label} layers")
    print(f"  Spacing: {args.spacing}")
    print(f"  Color: {args.color_mode}")
    print(f"  Output: {args.output}")

    total_points, lit_pixels = generate_text_ply(
        args.text, args.density, args.depth_mode, args.depth_layers,
        args.spacing, args.color_mode, args.font_size, args.output
    )

    print(f"\nDone! Lit pixels: {lit_pixels:,}, total points: {total_points:,}")

    # Show file size
    size = os.path.getsize(args.output)
    if size < 1024:
        print(f"File size: {size} bytes")
    elif size < 1024 * 1024:
        print(f"File size: {size / 1024:.1f} KB")
    else:
        print(f"File size: {size / (1024 * 1024):.1f} MB")

    # Show dimensions
    if total_points > 0:
        font = FONT_LARGE if args.font_size == 'large' else FONT_SMALL
        sample = next(iter(font.values()))
        char_width = len(sample[0])
        char_height = len(sample)
        gap = 1
        total_cols = (char_width + gap) * len(args.text) - gap
        depth_pts = args.density if args.depth_mode == 'cube' else args.depth_layers
        dim_x = (total_cols * args.density - 1) * args.spacing
        dim_y = (char_height * args.density - 1) * args.spacing
        dim_z = (depth_pts - 1) * args.spacing
        print(f"Dimensions: {dim_x:.2f} x {dim_y:.2f} x {dim_z:.2f}")


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
Generate PLY point cloud files with a grid of points for testing.

Usage:
    python generate_ply_grid.py --nx 10 --ny 10 --spacing 1.0 -o grid.ply
    python generate_ply_grid.py --nx 100 --ny 100 --nz 10 --spacing 0.5 --colors -o large_grid.ply
"""

import argparse
import random
import sys

def generate_grid_ply(nx, ny, nz, spacing, add_colors, color_mode, output_file):
    """Generate a PLY file with a grid of points."""

    points = []

    # Generate grid points
    for iz in range(nz):
        for iy in range(ny):
            for ix in range(nx):
                x = ix * spacing
                y = iy * spacing
                z = iz * spacing

                if add_colors:
                    if color_mode == 'gradient':
                        # Color based on position (gradient)
                        r = int(255 * ix / max(nx - 1, 1))
                        g = int(255 * iy / max(ny - 1, 1))
                        b = int(255 * iz / max(nz - 1, 1)) if nz > 1 else 128
                    elif color_mode == 'random':
                        # Random colors
                        r = random.randint(0, 255)
                        g = random.randint(0, 255)
                        b = random.randint(0, 255)
                    elif color_mode == 'height':
                        # Color based on height (z)
                        t = iz / max(nz - 1, 1) if nz > 1 else 0.5
                        # Blue -> Green -> Yellow -> Red
                        if t < 0.25:
                            r, g, b = 0, int(255 * t * 4), 255
                        elif t < 0.5:
                            r, g, b = 0, 255, int(255 * (1 - (t - 0.25) * 4))
                        elif t < 0.75:
                            r, g, b = int(255 * (t - 0.5) * 4), 255, 0
                        else:
                            r, g, b = 255, int(255 * (1 - (t - 0.75) * 4)), 0
                    else:
                        r, g, b = 200, 200, 200
                    points.append((x, y, z, r, g, b))
                else:
                    points.append((x, y, z))

    total_points = len(points)

    # Write PLY file
    with open(output_file, 'w') as f:
        # Header
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write(f"element vertex {total_points}\n")
        f.write("property float x\n")
        f.write("property float y\n")
        f.write("property float z\n")
        if add_colors:
            f.write("property uchar red\n")
            f.write("property uchar green\n")
            f.write("property uchar blue\n")
        f.write("end_header\n")

        # Vertices
        for pt in points:
            if add_colors:
                f.write(f"{pt[0]:.6f} {pt[1]:.6f} {pt[2]:.6f} {pt[3]} {pt[4]} {pt[5]}\n")
            else:
                f.write(f"{pt[0]:.6f} {pt[1]:.6f} {pt[2]:.6f}\n")

    return total_points

def main():
    parser = argparse.ArgumentParser(
        description='Generate PLY point cloud files with a grid of points.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Simple 10x10 grid (100 points)
  python generate_ply_grid.py --nx 10 --ny 10 -o small.ply

  # 100x100 grid with gradient colors (10,000 points)
  python generate_ply_grid.py --nx 100 --ny 100 --colors -o medium.ply

  # 3D grid 50x50x10 with height-based colors (25,000 points)
  python generate_ply_grid.py --nx 50 --ny 50 --nz 10 --colors --color-mode height -o cube.ply

  # Large stress test (1 million points)
  python generate_ply_grid.py --nx 1000 --ny 1000 --spacing 0.1 -o large.ply

  # Custom spacing in millimeters (for mm unit import)
  python generate_ply_grid.py --nx 100 --ny 100 --spacing 10.0 --colors -o mm_grid.ply
"""
    )

    parser.add_argument('--nx', type=int, default=10,
                        help='Number of points in X direction (default: 10)')
    parser.add_argument('--ny', type=int, default=10,
                        help='Number of points in Y direction (default: 10)')
    parser.add_argument('--nz', type=int, default=1,
                        help='Number of points in Z direction (default: 1, 2D grid)')
    parser.add_argument('--spacing', type=float, default=1.0,
                        help='Spacing between points (default: 1.0)')
    parser.add_argument('--colors', action='store_true',
                        help='Add RGB colors to points')
    parser.add_argument('--color-mode', choices=['gradient', 'random', 'height'],
                        default='gradient',
                        help='Color mode: gradient (position-based), random, or height (default: gradient)')
    parser.add_argument('-o', '--output', type=str, default='grid.ply',
                        help='Output PLY filename (default: grid.ply)')

    args = parser.parse_args()

    # Validate inputs
    if args.nx < 1 or args.ny < 1 or args.nz < 1:
        print("Error: Grid dimensions must be at least 1", file=sys.stderr)
        sys.exit(1)

    if args.spacing <= 0:
        print("Error: Spacing must be positive", file=sys.stderr)
        sys.exit(1)

    total_points = args.nx * args.ny * args.nz

    print(f"Generating {args.nx} x {args.ny} x {args.nz} grid...")
    print(f"  Total points: {total_points:,}")
    print(f"  Spacing: {args.spacing}")
    print(f"  Colors: {'Yes (' + args.color_mode + ')' if args.colors else 'No'}")
    print(f"  Output: {args.output}")

    # Warn about large files
    if total_points > 100000:
        print(f"\nWarning: Generating {total_points:,} points. This may take a moment...")

    count = generate_grid_ply(
        args.nx, args.ny, args.nz,
        args.spacing, args.colors, args.color_mode,
        args.output
    )

    print(f"\nDone! Generated {count:,} points in '{args.output}'")

    # Show file size
    import os
    size = os.path.getsize(args.output)
    if size < 1024:
        print(f"File size: {size} bytes")
    elif size < 1024 * 1024:
        print(f"File size: {size / 1024:.1f} KB")
    else:
        print(f"File size: {size / (1024 * 1024):.1f} MB")

if __name__ == '__main__':
    main()

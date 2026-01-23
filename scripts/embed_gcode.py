#!/usr/bin/env python3
"""
Converts a G-code file to a C header with embedded data.
Usage: python embed_gcode.py <input.gcode> <output.h> <variable_name>
"""

import sys
import os

def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <input.gcode> <output.h> <variable_name>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    var_name = sys.argv[3]

    # Read the gcode file
    with open(input_file, 'r') as f:
        content = f.read()

    # Get file size
    file_size = len(content)

    # Generate header
    header_guard = var_name.upper() + "_H"

    with open(output_file, 'w') as f:
        f.write(f"// Auto-generated from {os.path.basename(input_file)}\n")
        f.write(f"// Size: {file_size} bytes\n")
        f.write(f"#ifndef {header_guard}\n")
        f.write(f"#define {header_guard}\n\n")
        f.write(f"static const unsigned int {var_name}_size = {file_size};\n\n")
        f.write(f"static const char {var_name}_data[] = \n")

        # Write data in chunks to avoid line length issues
        chunk_size = 16384  # 16KB chunks
        for i in range(0, len(content), chunk_size):
            chunk = content[i:i+chunk_size]
            # Escape special characters for C string
            escaped = ""
            for c in chunk:
                if c == '\\':
                    escaped += '\\\\'
                elif c == '"':
                    escaped += '\\"'
                elif c == '\n':
                    escaped += '\\n'
                elif c == '\r':
                    escaped += '\\r'
                elif c == '\t':
                    escaped += '\\t'
                elif ord(c) < 32 or ord(c) > 126:
                    escaped += f'\\x{ord(c):02x}'
                else:
                    escaped += c
            f.write(f'    "{escaped}"\n')

        f.write(";\n\n")
        f.write(f"#endif // {header_guard}\n")

    print(f"Generated {output_file} ({file_size} bytes)")

if __name__ == "__main__":
    main()

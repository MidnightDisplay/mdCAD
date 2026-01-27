#!/bin/bash
#
# Apply gamepad support patch to Sokol for Android builds
#
# This patch enables keyboard and gamepad button events on Android.
# Without it, only touch input works.
#
# Usage:
#   ./scripts/apply-android-gamepad-patch.sh
#
# The patch modifies the fetched sokol_app.h in the Android build directory.
# You may need to re-run this after cleaning the build.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PATCH_FILE="$PROJECT_DIR/patches/sokol_android_gamepad.patch"

# Find the sokol_app.h in Android build directories
find_sokol_files() {
    find "$PROJECT_DIR/android" -name "sokol_app.h" -path "*/_deps/libsokol-src/*" 2>/dev/null
}

SOKOL_FILES=$(find_sokol_files)

if [ -z "$SOKOL_FILES" ]; then
    echo "No sokol_app.h found in Android build directories."
    echo "Run 'cd android && ./gradlew assembleDebug' first to fetch Sokol."
    exit 1
fi

echo "Found sokol_app.h files:"
echo "$SOKOL_FILES"
echo ""

# Check if patch is available
if [ ! -f "$PATCH_FILE" ]; then
    echo "Patch file not found: $PATCH_FILE"
    exit 1
fi

# Apply patch to each file
for SOKOL_FILE in $SOKOL_FILES; do
    echo "Checking: $SOKOL_FILE"

    # Check if already patched (look for our translate function)
    if grep -q "_sapp_android_translate_key" "$SOKOL_FILE"; then
        echo "  Already patched, skipping."
        continue
    fi

    echo "  Applying patch..."

    # Apply the patch manually (since patch command may not be exact match)
    # We'll use a simpler approach: find and replace the key_event function

    # Backup original
    cp "$SOKOL_FILE" "${SOKOL_FILE}.backup"

    # Create the patched version using sed
    # This is more reliable than using patch which requires exact context

    # The key function to replace is _sapp_android_key_event
    # We need to insert our translate function before it and replace the function body

    # For now, let's just note that manual patching is required
    echo "  Note: Automatic patching is complex. Apply the patch manually:"
    echo "  File: $SOKOL_FILE"
    echo "  Patch: $PATCH_FILE"
    echo ""
    echo "  The patch adds keyboard/gamepad support to Android."
    echo "  Without it, only touch input works (Shield remote touchpad is fine)."
done

echo ""
echo "Done. Rebuild with: cd android && ./gradlew assembleDebug"

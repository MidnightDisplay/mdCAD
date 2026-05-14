# Quick Task 260508-elz: Add new ImGui themes and sync Controls theme dropdown with applied theme

## Goal

Add the requested dark-mode ImGui themes plus a Cyberpunk 2077-inspired theme, and make the Controls window theme selector reflect the actual applied theme instead of a stale local copy.

## Task 1

- **files:** `src/ui/ui_theme.h`, `src/ui/ui_controls.h`, `src/app.c`
- **action:** Make `ui_theme.h` own the authoritative current-theme state, route all theme application through it, and have Controls read/write that state while keeping viewport clear-color syncing intact.
- **verify:** `cmake --build build-vulkan --config Release --target mdCAD`
- **done:** Theme dropdown selection always matches the actual applied theme, including startup and later theme changes.

## Task 2

- **files:** `src/ui/ui_theme.h`
- **action:** Add OneDark, Rose Moon, Nord, Tokyo Storm, Tokyo Night, Gruvbox Material Dark, and Cyberpunk 2077 theme palettes to the existing theme list.
- **verify:** `cmake --build build-vulkan --config Release --target mdCAD`
- **done:** All requested themes appear in the Controls window combo and compile cleanly.

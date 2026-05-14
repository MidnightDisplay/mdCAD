# Quick Task 260508-elz Summary

## Description

Added the requested ImGui themes and fixed the Controls window theme combo so it reflects the actual applied theme instead of a duplicated local index.

## What changed

- `src/ui/ui_theme.h`
  - Added authoritative current-theme tracking via `ui_theme_get_current()`.
  - Extended the theme list with OneDark, Rose Moon, Nord, Tokyo Storm, Tokyo Night, Gruvbox Material Dark, and Cyberpunk 2077.
  - Sized `ui_theme_names` with `UI_THEME_COUNT` so the enum and combo list stay aligned.
  - Added a shared zero-rounding helper used by the dark themes.
- `src/ui/ui_controls.h`
  - Removed the duplicated cached theme index from `ui_controls_state_t`.
  - The Controls combo now reads from `ui_theme_get_current()` every frame and writes back through `ui_theme_apply()`.
- `src/app.c`
  - Startup theme selection now goes through `ui_theme_apply(UI_THEME_VISUAL_STUDIO)`.

## Verification

- `cmake --build build-vulkan --config Release --target mdCAD` ✅

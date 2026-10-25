# ReyondBroom alpha 70

## Changes

- Optional Linux SDL2 backend, distributed separately from the native build.
- SDL retries without MSAA if the driver cannot create the requested window.
- New installations start at 1280 x 800.
- Resolution presets and custom dimensions in View > Resolution and Preferences > View.
- Resolution changes retain the editable MAP and restore ImGui after renderer restart.
- Includes merged PR #2: native Linux cursor shapes with image fallback.
- Linux updates retain the SDL/native variant. Sources remain separate archives.

## Verify

1. Extract the SDL archive into a separate writable folder. Start Launch-ReyondBroom.sh.
2. Check the initial window size, resize cursors and camera mouse capture.
3. Apply a preset in View > Resolution. Check the map and interface survive; restart and confirm persistence.
4. Repeat via Edit > Preferences > View > Resolution, including custom dimensions.
5. Check Russian text and clipboard paste under Wayland and X11.
6. Open an external Quake 1 mod. Create and move brushes, Undo/Redo, save a disposable map copy.
7. Open hotel02 in Quake III / FTE Custom; check patches, shaders, compilation and BSP inspection.
8. On Windows, check startup and editing after the merged Linux cursor changes.

The SDL build requires the system SDL2 runtime; see linux.md. Embedded child-window
mod preview remains Windows-only. Online work belongs in a subsequent experimental branch.

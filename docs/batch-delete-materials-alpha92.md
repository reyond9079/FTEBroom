# Alpha 92: batch deletion, transparency and asset workflows

Large brush selections now use a validated native deletion batch. The editor removes brush storage in descending slot order, updates retained document/render slots, and keeps unrelated geometry resident. Undo restores saved geometry and selection; bulk ID remapping and Redo avoid repeated history/index scans. Patch transactions retain the established full refresh path.

## Measured deletion

Windows, RTX 4060 Laptop GPU, the large E1M1 test copy with real WAD resources: 30,327 brushes, 189,772 faces and 56,616 document nodes. Timings cover the command and document update, not the subsequent render frame:

- Delete 1,024 brushes: 109 ms; Undo: 79–95 ms; Redo: 62 ms.
- Delete 8,192 brushes, including brush-owned containers: 197 ms; Undo: 640–701 ms; Redo: 106 ms.
- Geometry bounds and selection were verified after two Undo restorations. Deletion and Redo retained the base geometry epoch.

These measurements are operation latency, not a new camera-flight FPS claim. The source maps and WADs were read only; operations ran on editor-owned test copies.

## Material transparency

In MAP editing, Quake 1 clip, trigger, skip/nodraw/caulk, hint and related editor-only surfaces use 35% opacity. They remain selectable. Quake liquid opacity follows FTE's water/lava/slime/teleport alpha settings, including fallback to water alpha when a specific setting is empty.

WAD textures beginning with `{` retain index-255 cutouts. Opaque texels stay opaque; the whole texture is not arbitrarily faded. The cutout test runs before drag/selection opacity, so fading does not erase foliage. Masked materials bypass the opaque-only multi-material optimization.

Quake III static editor previews read `qer_trans`, diffuse-stage `alphaGen const`, `alphaFunc` (GE128, GT0, LT128), and blend functions. An ordinary diffuse/filter-over-lightmap shader stays opaque. `qer_trans` is an editor opacity override. This is an unlit material preview, not a complete animated multi-stage shader compositor; BSP/mod preview still uses the original engine shader. Resource reloads use fresh shader registrations.

## BSP models

Select whole brushes and use either File export or the viewport context menu's **Export selected BSP model...**. Enter an absolute path or a name relative to the current map folder. Surrounding quotes are accepted, `.bsp` is added when omitted, and **Choose folder...** navigates existing folders. Entering a directory uses `selected-model.bsp`. Missing directories, archive destinations and existing output files have separate messages.

Only selected brush geometry is compiled. The optional atlas and UV template retain their alpha90 limitations: static opaque diffuse materials, bounded atlas size, preserved collision and world coordinates. Quake BSP embeds its textures; painting the external TGA does not rewrite embedded pixels automatically.

Quake 1 compilation stages only referenced texture mip chains into one temporary `textures.wad`. This avoids compiler token limits and external Unicode WAD paths. The last matching WAD keeps priority, matching the editor. Source archives are unchanged.

Dragging a BSP from the file manager into the viewport assigns `func_model` in Quake 1/FTE Custom, and `misc_model` in Quake III. FTE can display external BSP models. A target compiler's model-format support is separate: stock q3map2 does not import every format FTE can display.

## BSP in Model editor

Double-click a BSP in the file manager or choose **Open in model editor** from its context menu. The preview displays static BSP geometry with its materials, orbit/free camera, floor grid and origin axes. Bounds and dimensions appear in the inspector. BSP viewing is read only; mesh transforms, animation, bones and model-converter export are not offered for BSP files. Source MAP editing and recompilation remain the way to modify BSP geometry.

## Face selection

When Shift is held and brushes are already selected, hover outlines, clicks, face painting and coplanar/double-click selection stay within those selected brushes. Foreground unselected brushes do not light up. Without Shift, the Face tool can still pick another brush.

## Packed LMP images

Double-click an LMP in a PAK/PK3 or use **Open LMP viewer**. Palette files open in the palette viewer; Quake pictures open in the image viewer. Thumbnails, zoom and fit work for indexed pictures, conchars and colormaps. The palette is read from the same archive when present, otherwise from the mounted game resources. Unsupported binary LMP data is reported rather than guessed as an image.

All 56 LMP pictures in the tested LibreQuake PAK decoded successfully. Unit checks also cover pixel transparency, fonts, colormaps, invalid/truncated data, material shader parsing and unchanged compiler WAD mip bytes.

## Checks to repeat

1. On a large map, delete a large brush group. Orbit and fly around the result; deleted faces and grids must not remain. Undo, move the restored group, Undo again, and Redo deletion. Save/reopen a test copy.
2. Select one of two nearby brushes. Hold Shift over each: only the selected brush should show a face outline. Test clicks, paint selection, double-click and coplanar flood. Release Shift and verify ordinary Face selection.
3. Right-click selected geometry and export a BSP. Try a bare filename, a quoted path with spaces, a chosen folder, an invalid folder and an existing model. Check that the original model is protected. Enable the atlas and verify both atlas and UV template images.
4. Drag the exported BSP onto a test map. Check the assigned entity class, displayed model, position and Undo/Redo. Then open the BSP in Model editor by double-click and context menu; test textured display, orbit, fly, zoom, reset camera, floor/axes and closing it. Test Quake 1 and Quake III separately.
5. Compare clip/service faces, foliage cutouts, glass and liquids against their intended material settings. Select and drag them: texture and transparency must remain. For Quake III test both `qer_trans` and alpha-tested grates, plus an ordinary opaque lightmapped wall.
6. Open palette, picture, font and colormap LMP entries inside a PAK; inspect thumbnails and image zoom. A malformed LMP should show an error without a crash.
7. Repeat basic editing, deletion and asset browsing on Windows, native Linux and Linux SDL2. Arch/KDE Wayland tests use a virtual compositor/software renderer and do not establish physical Linux GPU FPS.

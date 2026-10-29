# Quake II and Half-Life

Choose the game in first-time setup, or **Edit → Preferences → Game profile**.
Changing profiles closes the current scene after the unsaved-changes prompt.
Original game data is required and is not distributed with the editor.

## Quake II

Select `baseq2`, a Quake II installation containing `baseq2`, or a mod directory.
For a mod beside `baseq2`, base resources load first and the mod overrides them.
The palette comes from `pics/colormap.pcx`, including copies in PAK archives.
WAL materials are decoded into a persistent editor cache; PCX and WAL images can
also be opened from the file manager. Entity definitions default to `quake2.fgd`.
Use an optional custom FGD for mod-specific classes and model paths.

MAP files use Valve 220 axes and retain contents, surface flags and surface values.
**Compile** uses the bundled, separately pinned ericw-tools Quake II toolchain:
QBSP (`-q2bsp`), VIS and LIGHT. Stage selection and extra arguments are available
under **Setup compile**. The result is IBSP version 38; **Show BSP** displays it
inside the editor. Native Quake II game DLL execution is not implemented by this
profile; test gameplay with your Quake II engine through a custom Run task.

MD2 models and PCX skins support file-manager previews, entity thumbnails and
Model editor inspection. MD2 uses vertex animation and has no skeletal bones.
Vanilla Quake II has no generic arbitrary-model entity. Model drag placement
requires a mod definition for `misc_model`; otherwise the editor explains the
limitation instead of inserting a Quake 1 `func_model` that the game cannot use.

## Half-Life and Counter-Strike 1.5

Select `valve`, `cstrike`, an installation containing either, or your mod directory.
A sibling `valve` supplies base assets beneath the selected mod. WAD3 textures
keep their per-texture palettes; the Quake 1 `gfx/palette.lmp` requirement does not
apply. Use **Add WAD(s)** for additional archives. WAD3 image import and new WAD3
creation are supported. The default entity definitions are Half-Life FGD, or the
Counter-Strike additions when FreeCS is selected. Definitions found in the mod
take precedence; a manually configured definition file has highest priority.

MAP output uses Valve 220 axes and `"mapversion" "220"`. The bundled SDHLT chain
runs CSG, BSP, VIS and RAD, producing BSP version 30. Geometry enables both CSG and
BSP; advanced arguments for these two stages are independent. CSG embeds the used
textures, so the compiled result does not depend on desktop WAD paths.

Choose **FreeHL** for Half-Life or **FreeCS** for Counter-Strike 1.5 before loading
resources. The editor installs one separately named runtime PK3 into the selected
mod. Existing files are not replaced. After compilation, **Preview mod** runs that
open implementation: embedded on Windows, in a separate game window on Linux.
These runtimes need the corresponding original game assets and do not execute
arbitrary native Half-Life DLL mods. Their gameplay compatibility follows FreeHL
and FreeCS, rather than promising full GoldSrc compatibility.
Switching runtimes deactivates only byte-identical editor runtime packages;
modified or unrelated mod files are retained.

GoldSrc studio MDL v10 supports textured thumbnails, skins and animated Model
editor inspection. Enable **Show bones** to draw joints over the model. Texture
companion `*T.mdl` files must remain beside the model. FTE can prefer replacement
models sharing a basename; keep this in mind when comparing MDL and MD2 files.
Since alpha 95, the complete studio rig can be moved, rotated and uniformly
scaled directly. Native MD2 and studio MDL export are available; see
[native model export](native-models-alpha95.md) for format limits.
Dropping a studio MDL into a Half-Life map creates `cycler`; sprites use
`cycler_sprite`. Other FTE model formats remain inspectable but are not silently
inserted as unsupported native Half-Life entities.

## Shared tools and limits

Brush editing, material assignment, CSG, UV editing, entities, Undo/Redo, saving,
archive navigation and background compilation use the same editor workflows.
Patch and FTE Engine terrain creation are disabled in these classic game profiles.
Selected-brush BSP export uses each game's compiler with no-fill mode and does not
open the general compilation window. UV inspection supports all four BSP families.
Atlas generation currently supports Quake 1 and Quake III only; the option is
disabled for Quake II and Half-Life rather than producing an invalid atlas.

The converter's existing FTE-compatible output formats remain available. Alpha 95
adds native MD2 and GoldSrc studio MDL exporters. A Quake MDL output is not a
Half-Life MDL. BSP inspection is static geometry, not a replacement for gameplay testing.

## Verification

The release tests use original, generated assets, not copied retail game content:
sealed six-brush rooms, WAL/PCX, WAD3 and a two-joint studio MDL fixture. Both full
compiler chains were exercised on Windows and Linux. The editor tests cover
resource mounting, game entity sets, valid BSP output, deletion, Undo/Redo and
save. Parser tests additionally cover malformed input and palette/UV extraction.
The Linux UI test bed uses Arch with KDE Plasma 6.8 and both native and SDL2
Wayland backends; its software renderer is not a hardware FPS benchmark.

Additional checks validated selected-only BSP exports for both new games and a
Quake II surface with nonzero flags/value (`2`, `17`) through save and compilation.
FreeHL and FreeCS started the generated BSP in a private dedicated-server smoke
test and shut down without the renderer-cleanup crash. This is not a full retail
gameplay or arbitrary-mod compatibility test. PAK and PK3 deletion retained other
members and created no new backups. Sound gain changes retained live playback.

The existing large-map camera route with real WADs (30,327 brushes) measured
109.85 FPS at 1920×1080 and 105.62 FPS at 2560×1440 on the Windows test machine.
Selecting 8,192 brushes measured 50.48 and 48.28 FPS respectively. These are short
route measurements, not a guarantee for every map or operation. The rendering
quality settings were retained. The complete engine patch stack was applied to a
fresh pinned source tree and matched all 48 affected files in the tested engine.

For your installation, check:

1. Load base assets and a real mod; verify that mod materials override base data.
2. Create, move, texture and save a brush; reopen the map and undo/redo edits.
3. Compile with all stages; inspect textures, light, collision and brush entities.
4. Open MD2/PCX or studio MDL, change skin, play animation and inspect bones.
5. For Half-Life/CS, launch Preview mod with legitimate game assets and verify
   the particular gameplay features your map uses.
6. Reopen Quake 1 and Quake III maps to check profile and resource switching.

See [compiler/runtime pins and reproduction](../tools/legacy-games/README.md).

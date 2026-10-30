# ReyondBroom FTE-native map editor

[**Join our Discord**](https://discord.gg/eDMf53Kny)

**Alpha 100: Escape lighting controls on Linux native/SDL2 and Windows; softer RTL presets with adjustable baked fill for a closer lightmap match.** [Lighting guide and checks](docs/mod-lighting100.md). [Usage, limits and verification](docs/native-models-alpha95.md) · [Game profiles](docs/legacy-games-alpha94.md) · [Plugins](docs/plugins.md) · [Model editor](docs/model-editor.md).

Automatic Blender download: [English](docs/alpha62.md).
Model import, texture transfer and skin preview: [guide](docs/model-import64.md). Tools adapt to horizontal and vertical panels.

[**Illustrated user manual**](docs/manual.md) · [**Linux x64 download**](https://github.com/reyond9079/ReyondBroom/releases/download/v0.5.0-alpha.104/ReyondBroom-0.5.0-alpha.104-Linux-x64.tar.gz) · [**Linux SDL2 x64 download**](https://github.com/reyond9079/ReyondBroom/releases/download/v0.5.0-alpha.104/ReyondBroom-0.5.0-alpha.104-Linux-SDL2-x64.tar.gz) · [Linux requirements](docs/linux.md)

[Quake 1 / LibreQuake / PAK / entity models](docs/alpha29.md)

[FTE Custom game — game profile](docs/when-the-sky-went-dark.md)

[**Download Windows x64 - 0.5.0-alpha.104**](https://github.com/reyond9079/ReyondBroom/releases/download/v0.5.0-alpha.104/ReyondBroom-0.5.0-alpha.104-Windows-x64.zip)

An independent map editor built on FTE Quake, csaddon and Dear ImGui. Menus and tools surround a live engine viewport, with a persistent inspector and one to four perspective/orthographic panes. Profiles support Quake 1, Quake II, Quake III Arena, Half-Life and FTE Custom game. Quake 1 supports standard and Valve 220 MAP; Quake 3 retains its shaders and patches.

**Alpha.60:** four-step setup with theme previews, saved fullscreen mode, FTE Custom offset mapping and reduced resource-reload stalls. [Usage and checks](docs/alpha60.md) · [Custom GLSL](docs/alpha59.md).

[**New tools and usage**](docs/alpha21.md) · [Workspace settings](docs/alpha17.md) · [**Complete test checklist**](docs/checks-alpha21.md)
![ReyondBroom editor overview](docs/images/editor-overview.png)

## Available features

- Dark palette, dockable/resizable panels and tabs, 100–200% UI/font scaling, saved layouts, panel locking and reset.

- Quake 1: WAD2/palette resources, 99 entity classes, standard/Valve 220 MAP, BSP29/BSP2 builds and free-camera/play preview. [Guide](docs/quake1.md).
- LibreQuake installer, packed WAD/MAP resources and mod model thumbnails. [Guide](docs/alpha29.md).
- Quake II: WAL/PCX and PAK resources, MD2 models, Valve 220 MAP and an isolated ericw-tools IBSP38 compile chain. [Guide](docs/legacy-games-alpha94.md#quake-ii).
- Half-Life / Counter-Strike 1.5: WAD3 palettes, studio MDL, FGD, SDHLT BSP30 compilation and FreeHL/FreeCS runtime installation. [Guide and compatibility limits](docs/legacy-games-alpha94.md#half-life-and-counter-strike-15).
- Quake 3 resource setup; loose textures, PK3 archives and shader previews. Resources are mounted read-only.
- Native file navigation for Open/Save As, dirty-state guards, MAP persistence, autosave and transaction-based Undo/Redo.
- WASD/QX navigation, relative mouse look, orbit/pan, saved viewpoints, orthographic views, linked cameras and resizable layouts.
- Grid 0.125–256, snapping, visible surface/background grids and configurable grid opacity.
- Brush, face, patch and point-entity selection; depth drilling, paint selection, groups, selection bounds/dimensions and locked-object visualization.
- All/Invert/Siblings/Touching/Inside/Tall selection and selection by MAP line.
- Box creation, mixed-object movement, independent duplication of nested groups/brush entities, deletion and clipboard MAP fragments.
- Material browser with folder/used filters, search and usage counts; material selection/replacement, UV fields, surface attributes, alignment/Fit and texture transfer.
- Keyboard rotation/flip and interactive Rotate with movable pivot, angle snapping and texture lock.
- Shift-drag Extrude; Shift+Ctrl Split; Shift+Ctrl+Alt Stamp, with scene previews, cancellation and single-step Undo.
- Clip with movable two/three-point cuts, face-plane double click, translucent discard preview and validated multi-brush Apply.
- CSG Merge/Subtract/Intersect/Hollow, vertex/edge/face editing, Move Face, Scale/Shear and point-based convex brush construction.
- Cylinders/cones, hollow cylinders, UV spheres/icosahedra, stairs, arches/spandrels, scalable circles and grouped multi-part shapes.
- ENT/DEF/FGD entity definitions, 53 bundled Quake 3 classes, drag placement, properties/defaults/flags/colors, brush entities and target/killtarget links.
- Groups, layers, Hide/Isolate/Show All, locking, class/tag/display filters, scene context menus and omit-from-export MAP output.

- Linked groups, relative-transform synchronization, protected entity properties, separation and extraction.
- Face-to-patch conversion and shared control-point editing with Bezier subdivision.
- Straight/Arc/S-bend Sweep with cap handles, taper, iterations and live preview.
- Graphical UV offset/scale/rotation/shear, origin handles, snapping and subdivisions.
- Repeat command sequences; live Issues with undoable fixes; leak paths and portal overlays.
- Compile/Launch task profiles, streamed output, Stop/Rerun, MAP and OBJ/MTL export.
- Configurable backup rotation/recovery and persistent context-aware keyboard shortcuts.

## Start

Extract the ZIP completely and run **ReyondBroom.exe**. Select **Quake III Arena**, choose your **baseq3** directory and load resources. Retail assets are not included. The editor runs in its own directory and does not write into baseq3.

Run **Launch-Checks.cmd → B** for the new brush workflows. Use copies of maps while testing. The [combined checklist](docs/checks-alpha21.md) covers new features and regressions, including real cross-application clipboard use and Windows DPI scaling.

## Main controls

Click to select; drag a selected object to move. With no selection, drag empty space to create a box one grid cell thick. Normal movement follows XY; Alt constrains movement to Z. Shift+click selects a face, while Shift-drag on a selected whole brush resizes its face. In 2D, grab the face's projected edge.

Ctrl-drag duplicates; Ctrl+D duplicates in place. R activates Rotate, C activates Clip. Escape cancels a gesture; Shift+Escape leaves the modal tool. Delete removes selected objects; Ctrl+Z/Ctrl+Y undo/redo. The inspector remains visible; function keys are not assigned editor shortcuts. Text entry and dialogs capture keyboard input without moving the camera.

The plain mouse wheel changes grid size by default; Preferences can switch to Ctrl+Alt+wheel. The Material tab applies a clicked material to selected faces/brushes. Edit offsets, X/Y scale and angle there; nonzero negative scales mirror the texture. Texture rotation preserves existing axis skew.

## Limits

The implementation retains a 64-face convex-brush limit, fixed patch preview tessellation and bounded entity-model descriptor support. See [practical limits](docs/alpha21.md#practical-limits).

The MAP writer uses Valve 220 texture axes and Quake 3 patch data. q3map2 is bundled; custom compiler paths are supported. Windows x64 and native Linux x86-64 packages are available. [Linux dependencies and platform differences](docs/linux.md).

## Build

Install Python 3, Git, PowerShell and MSYS2 at `C:/msys64`, with MinGW64 GCC/G++/make/zlib/libpng/libjpeg-turbo and standard MSYS tools. Run `scripts/build-windows.ps1`. The script fetches pinned FTE/ImGui sources, applies all engine patches and builds the engine, addon and plugin into `out/ReyondBroom-Windows-x64`.

See [architecture](ARCHITECTURE.md), [verification](TESTING.md), [shortcuts](docs/shortcuts.md) and [third-party notices](THIRD-PARTY.md). The combined editor includes GPL-3.0-or-later adaptations; original notices are retained. ImGui is MIT. Binary packages include corresponding source and upstream licenses. Preserve these when redistributing.

Experimental small-map GPU lighting: [usage, measurements and limitations](docs/gpu-lighting101.md).

Experimental Quake 1 **GPU lighting directly on MAP**, with direct light and one diffuse bounce, no BSP/VIS compilation: [usage and limitations](docs/gpu-map-lighting103.md).

Classic MAP material fixes, compile placeholders and display brightness: [alpha.104 guide](docs/materials104.md).

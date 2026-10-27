# ReyondBroom user manual
[Keyboard reference](shortcuts.md) · [Release history](../README.md)

This guide covers the editor's available workflows. Screenshots show actual editor
sessions; older screenshots may use earlier labels or layouts. Game assets are not
included. A profile selects map format, authoring resources, entity definitions and
build tools; it does not convert arbitrary game logic into another game.

## Installation and first-time setup

Extract the complete distribution into a writable folder. On Windows, run
`Launch-ReyondBroom.cmd`: the editor opens in a maximized, decorated window.
Keep the tools, fonts and runtime directories with the executable.

On a fresh installation the first-time wizard presents language, game profile,
then resources. Select a language first; it can later be changed in **View →
Language**. Choose the game and configure its authoring directory. Press **Load
resources and start**. Resource errors leave the setup available for correction.
Existing configured installations retain their paths and skip the wizard.

**Quake III Arena:** select a `baseq3` authoring folder containing PK3 archives,
textures or shader scripts. Quake 3 shaders and patches are available.

**Quake 1:** select a self-contained mod folder (such as `id1`), or download LibreQuake and enable
**Use LibreQuake mod**. Additional WAD2 archives and custom
entity definitions are optional. The selected mod supplies both authoring and runtime resources. Standard and Valve 220 MAP and BSP29/BSP2 are
supported. Quake 1 material resources come from WADs; patches are unavailable.
See [Quake 1 resources, compilation and limits](quake1.md) and
[LibreQuake and PAK setup](alpha29.md).

**FTE Custom game:** use a Quake 3 authoring library and optionally a separate
runtime mod. Entity-definition files or QuakeC source descriptions improve the
entity browser but are not needed to draw brushes. The runtime mod is required
for **Preview mod**, not for an ordinary BSP build. Existing maps using the old
`when-the-sky-went-dark` identifier retain their profile.

Change profile through **Edit → Preferences → Game profile → Switch profile**.
The editor offers to save dirty work, closes the old document and creates a
starter scene. Opening a MAP can select its profile from ReyondBroom or supported
TrenchBroom header comments. Ambiguous geometry alone does not choose a game.

## Workspace and navigation

Drag panel tabs to dock them; drag borders/dividers to resize. The default file
manager occupies the area below the viewport, leaving the tool strip and
inspector accessible. Preferences offer layout reset, panel locking and UI scale.
The default scale is 125%. **View → Interface theme** offers Modern Dark, Modern
Light, Windows 95/98 and Windows 95/98 Dark. Appearance and layout persist.
Seven UI languages are available; some help, engine diagnostics and game-authored
descriptions remain in their original language.

Choose one to four views and perspective, XY, XZ or YZ orientation in the toolbar.
Use right-drag to look and WASD to fly; Q/X move vertically, Shift increases speed.
Camera preferences provide navigation settings. Orbit, pan, saved viewpoints and
linked orthographic views are available through the view controls. Text inputs
capture typing so editing a property does not move the camera.

## Files, recent maps and recovery

**File → New** asks for a profile and destination. **Open** and **Save As** use a
folder browser: click folders to enter them and use **Up** for the parent. Saved
maps default to their own directory; unsaved maps use the authoring `maps` folder
when configured. Double-clicking a MAP in the Mod file manager uses the same
Save / Don't Save / Cancel guard. Cancel or a failed save keeps the current map.

The startup list shows one row per map with its last profile. Click a star to
favorite a map; the star filter shows favorites only. Missing paths remain listed
but cannot be opened. **Create new map** starts a new document.

Use Ctrl+Z/Ctrl+Y for Undo/Redo. Geometry tools commit complete operations; Escape
cancels an active gesture. Autosave and rotating backups are configured in
**Edit → Preferences → Backups**. Restore backups as separate maps before replacing
important work. Crash reports, when the crash handler can run, record diagnostic
context; they are not a replacement for autosave.

## Grid, selection and movement

The grid ranges from 0.125 to 256 units. The wheel changes grid size; Preferences
can select the modifier-based alternative. Grid visibility, snapping and opacity
are separate settings. **View → Grid → Grid rotation** rotates the XY construction
grid around Z in whole degrees. Enter an integer or drag the slider. **Reset grid
rotation** restores zero. The angle is saved. It affects snapping and new XY-view
or perspective shapes; side views retain their editing plane.

Brush surface grids fade from 512 to 1,024 world units by default. In **Map → View options → Antialiasing and texture filtering**, change **Grid fade start** and **Grid draw distance**. Distance `0` disables the limit. These saved settings affect surface grid lines only; brush edges, textures and geometry remain visible.

Click an object to select it. Drag selected objects to move them in XY; hold Alt
to constrain movement to Z. Perspective dragging scales with distance: nearby
objects move less for the same cursor displacement. Vertex dragging follows the
same principle. The original disappears during a move preview; duplication keeps
it. Ctrl-drag duplicates, Ctrl+D duplicates in place, Delete or Backspace deletes.
Shift-click selects a face. Shift hover outlines the candidate face in yellow.

Selection tools include All, Invert, Siblings, Touching, Inside, Tall and selection
by MAP line. Depth selection helps reach overlapping objects. Hidden or locked
objects obey the current filters. Alignment guide rays stop at visible unselected
brushes and mark contact in red; they do not collide with patches or model meshes.

## Brushes and geometric tools

With the default tool and no selection, drag empty space to create a box one grid
cell high. Choose a primitive in Tool options and set its dimensions/subdivisions
before creation. Available shapes include cylinders, cones, hollow cylinders,
spheres, stairs, arches, spandrels and circles. Grouped multi-part shapes remain
editable as their component brushes.

**Rotate (R):** drag rotation handles; reposition the pivot and adjust snapping.
Keyboard transforms include rotate and flip. **Scale / Shear** use their handle
tools. Texture lock controls whether material alignment follows the transformation.

**Vertex (V), Edge (E), Face:** select handles, drag to edit and use Alt for vertical
motion. Invalid or nonconvex results are rejected. Orthographic views keep the
hidden coordinate fixed. Brush geometry is convex and limited to 64 faces.

**Extrude:** Shift-drag a selected brush face or its silhouette edge. Shift+Ctrl
splits; Shift+Ctrl+Alt stamps. A preview appears before release; Escape cancels.
Silhouette picking chooses a nearby face when exact face targeting is difficult.

**Clip (C):** place two points for a view-dependent plane or three points for an
explicit plane. Move the points to refine the cut. The discarded portion is
translucent. Select the keep side/mode in Tool options and Apply; Ctrl+Enter uses
the clipping action. Double-click a face to use its plane. Undo restores the
original brushes. See [clip and geometry workflows](alpha21.md).

**CSG:** Merge creates a convex result when possible; Subtract removes cutter
volumes, Intersect retains common volume, Hollow creates walls. Check selection
and preview before applying. More detail: [advanced geometry](alpha21.md).

**Sweep:** choose a source shape and Straight, Arc or S-bend, then set caps, taper,
iterations and handles. Preview before applying; undo the result as one operation.
**Repeat** replays a recorded command sequence; check the selected objects first.

## Bevel and terrain generator

**Edit → Bevel** supports entire brushes, the outer contour of selected faces,
selected edges and selected vertices. Set radius, segment count and profile.
Profile 0 is flat, 0.5 round, approaching 1 square. Clamp limits overlap inside
each brush independently. Preview in the scene, then Apply or Cancel. Excessive
complexity can exceed the 64-face limit; reduce segments or selected edges.

**Edit → Terrain generator** replaces selected brushes with convex terrain cells,
or modifies selected patches. Set U/V subdivisions, height, intensity, frequency,
roughness, seed and axis/side. Triangular cells are the default. Brush generation
uses each source bounding box; it does not preserve arbitrary original topology.
Patch U/V counts are odd values from 3 to 31; reducing them approximates the shape.
Preview, Cancel and one-operation Undo are available. The output limit is 2,048
brushes. This generator is distinct from FTE Engine terrain.

## Patches and UV editing

Quake 3 and FTE Custom game support curved patches. Create a patch through the
patch commands/face conversion, select it and use **Control Point (P)**. Drag its
control points, subdivide or change the grid in the terrain generator. Material
assignment and texture settings operate on patches too. Quake 1 hides Control
Point and does not support patch output. Keep patches when exporting Quake 3 MAP.

The graphical UV editor provides offset, scale, rotation and shear, origin
handles, snapping and subdivisions. Select a face, open UV editing and adjust
handles or fields. Use Undo to restore the previous mapping. See the
[UV and patch guide](alpha21.md).

## Materials and WAD import

Click a material to apply it to the selected brush/faces/patch, or set the material
for the next brush when nothing is selected. Material properties include offsets,
X/Y scale, angle and face attributes. Nonzero negative scales mirror textures.
Alignment, Fit and transfer actions help reuse an existing face's mapping.

Filter by name, collections, used materials and usage count. Collection selection
has its own search. **Advanced search** adds minimum/maximum width and height;
zero means unlimited, equal bounds mean exact dimensions. Shader dimensions refer
to their preview image. Drag the divider above the browser to resize it independently
from properties; adjust thumbnail size for larger previews.

For Quake 1, use **Add WAD(s)** or drop WAD files into the editor. Dropping a supported
image into the Material browser opens WAD import: choose the destination WAD and
texture name, then convert to supported dimensions and the Quake palette. Existing
archives retain backups. Compilation stages referenced WADs under the build job
using ASCII paths, including WADs originally loaded from Unicode desktop paths.
Merely copying a WAD beside a MAP does not register it: load it in Material first.

## Entities, groups and layers

The entity browser uses the profile's ENT/DEF/FGD definitions. Drag a tile onto the
scene to place an entity. With an entity selected, choosing another class changes
its class. Edit key/value properties, defaults, spawn flags and colors in Entity.
Model thumbnails need valid model/material paths in the mounted resources.
Definition-based previews do not run `progs.dat` to discover arbitrary spawn logic.

Convert selected brushes to a brush entity using entity commands. Groups organize
objects; layers control visibility and locking. Hide, Isolate and Show All combine
with class/tag filters. Class names, point bounds, group bounds and brush-entity
bounds can be enabled in View. Target and killtarget overlays show entity links.
Omit-from-export settings retain editor-only organization without compiling it.

Linked groups synchronize instances through relative transforms; protected entity
properties allow per-instance differences. Separate/extract instances when they
should become independent. See [linked-group workflow and limits](alpha21.md).

## Mod file manager, models and sounds

Open the file manager from **Workspace**. It supports folder trees, tiles or lists,
thumbnail size, Up, Refresh and opening a directory in the operating-system file
manager. FTE Custom game exposes **Mod files** and **Map sources / authoring assets**
as separate roots. Switching roots does not change the active profile or runtime mod.

Right-click files for Copy, Cut, Paste, Rename or text editing. External drops copy
files; they do not delete the source. Dropping a model on the scene copies it into
the mod and creates `func_model` at the drop position. PAK archives appear as folder
icons with an archive stripe. Browse, import, replace and rename members; archive
writes retain backups. Cross-boundary Cut and entire packed-folder copying are not
supported. Use Copy to extract/import. See [PAK operations](alpha34.md).

Double-click a model for **Model viewer**. Left-drag orbits, the wheel zooms. Choose
an animation, play/pause or scrub the frame slider and set playback FPS. Skeletal
models offer bones; vertex-animated MDL/MD3 models do not have skeletal bones.
Referenced skins and shaders must be available. The viewer does not run game events.

Double-click WAV, OGG, MP3 or FLAC for **Sound editor**. Inspect the waveform, select
a time range, set volume/speed and preview playback. Enable **Trim to selection**
to retain only the chosen interval. **Save with backup overwrites the original**,
including PAK members. Speed changes pitch too. Processing uses 44.1-kHz stereo;
WAV export retains the original channel count/rate, compressed formats are reencoded.
Loop/cue metadata is not preserved. Keep the backup for sounds relying on it.

## Build, BSP inspection and mod preview

Save the map once. **Build...** configures compiler paths, Geometry/BSP, VIS, LIGHT
and advanced arguments per stage. **Build** starts immediately with saved settings.
The job works on a snapshot beside the map under `_ftebroom-build`; the original MAP
is not rewritten. A viewport overlay shows stage/status; the log records compiler
output. Cancel stops the job and retains the previous successful result.

By default successful Build opens **Compiled BSP (view only)** in the viewport.
Use **Back to editing** to resume MAP editing. It does not start the mod. The
optional **Open mod preview after Build** setting explicitly opts into mod launch;
older implicit opt-ins are reset on upgrade. VIS/LIGHT-only builds require the last
successful BSP from the identical exported geometry.

**Preview mod** runs configured game resources and game logic separately from MAP
editing. Choose Free camera or Play mod; Escape releases captured input and Back
to editing exits. This requires a compatible runtime mod and its assets. Authoring
textures do not automatically become runtime mod assets. Shader time and camera
effects can differ between editor, BSP and the actual mod.

Run/task profiles support export, compiler invocations, launch, Stop and Rerun.
MAP and OBJ/MTL export are available. Issues inspect map problems and offer fixes;
leak paths and portal overlays help inspect compiler output. Use a sealed room and
player start when testing a first BSP build.

## FTE Engine terrain

Available only in **FTE Custom game**: **Edit → Create FTE Engine terrain**, or the
scene context menu's **Create FTE Engine terrain here**. Set origin and ground
material, then Add. A two-by-two section area is added to the existing map.
The FTE Engine terrain panel provides sculpting, smoothing, flattening, material
painting, water height and holes, with brush radius, strength and amount.

Each stroke and creation participates in Undo/Redo. Exit editing, change tool or
leave the terrain panel to release the terrain brush. Save keeps section data beside
the MAP; move both together. Save As copies sections. This is an FTE heightfield,
not a Q3 patch or BSP brush: Q3Map2 cannot compile it, and ordinary BSP Build is
disabled for terrain documents. Quake 1 and Quake 3 profiles cannot create it.

## Updates, diagnostics and practical limits

Windows **Help → Updates** checks releases, downloads a verified package and
installs after normal exit. It retains settings and user files; staging/rollback
data remains under `updates`. See [update behavior](alpha30.md). Linux packages
must currently be updated manually. Do not run Windows installers through Wine
to update a native Linux installation.

Check [shortcuts](shortcuts.md) and **Preferences → Keyboard** for current bindings.
Function keys are reserved rather than assigned editor actions. Alpha features
remain bounded: convex brushes have 64 faces, previews/large models can cost FPS,
language coverage is partial, and sample editing does not preserve loop metadata.
Back up real projects before trying new versions. For a bug report include version,
profile, editing/BSP/mod mode, reproduction steps and the log/crash report.

## Unity Sync

**Tools → Unity Sync** exports brushes, tessellated patches, UVs and static material previews into a Unity project. Choose separate objects or a combined mesh, colliders and unit scale. [Complete workflow, limitations and checks](alpha51.md).

## Texture folder imports

In Quake 1, **Material → Texture folders / compile WAD…** imports image folders recursively with a selectable palette, background conversion and saved rebuild settings. Generated WADs are registered for both editing and map compilation. [Workflow and checks](alpha52.md).

## Extended Bevel

Bevel supports inward and asymmetric edge profiles and retained offcuts. [Usage, limits and checks](alpha53.md).

## Symmetry settings and existing copies

**Edit → Symmetry → Settings…** controls placement, guides and relinking. [Workflow, limits and checks](alpha54.md).

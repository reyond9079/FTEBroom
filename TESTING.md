# Prototype checks

## Linux native cursor, 2026-10-25

- Wayland with `WAYLAND_DEBUG=1` shows `wp_cursor_shape_device_v1.set_shape` calls with the pointer-enter serial for arrow, text, hand and resize cursors. No `unknown cursor shape` or other protocol errors appeared; the compositor drew the themed pointer.
- `scripts/test-linux38.sh` passes `linux_native_cursor_owner`, `linux_cursor_fallback_generated`, `linux_cursor_shape_text`, `linux_cursor_shape_ew`, `linux_cursor_hidden_for_camera` and `linux_native_cursor_after_camera` under Xvfb. ImGui no longer draws a second pointer.
- The plugin writes ImGui-atlas TGA fallbacks for compositors without shape support. `scripts/test-linux39.sh` was updated for the Wayland path, but Weston was unavailable in this environment; the Wayland check above was performed manually.

## Linux surface-grid depth, 2026-10-25

- The `ftebroom/surfacegrid` and `ftebroom/brushedges` shaders no longer request `polygonoffset`. FTE's default bias is 25 depth LSBs; with the editor near plane at 4 and no far plane that is roughly 100 world units at 16k view distance, so grid lines on a distant wall reached in front of a pillar only 80 units closer. Both overlays draw after world geometry, and the default `GL_LEQUAL` depth test keeps coplanar surfaces visible without the bias.
- Rebuilt `csaddon.dat` with the pinned FTEQCC and tested on Linux Wayland with an AMD Radeon RX 570 / Mesa 26.2.5. The test map places a 16k gridded wall behind a foreground pillar; the pillar now renders cleanly with the grid visible on the wall behind it. Windows rendering was not retested for this change.
- `scripts/test-grid-depth-linux.sh <deps>` renders the fixture under Xvfb/llvmpipe and counts unobstructed pillar pixels. It passes with this change and reports 220 instead of 345 blue pixels when `polygonoffset` is restored, so the regression is exercised.
- `scripts/build-linux.sh` now packages the matching 0.5.0-alpha.68 version instead of alpha.67.

## Alpha.30 verification, 2026-10-22

- Native Windows/OpenGL integration: nine assertions cover saved retro theme, classic styling, horizontal/diagonal OS cursors, the WM_DROPFILES route, import destination dialog, image resampling/WAD append and MDL/MD3 preview creation. Separate screenshot pixel assertions confirm both MD3 surfaces and both MDL skin colours actually rendered. Modern and retro interface screenshots were inspected after the winding/bevel fix; floating panels at 125% scale were inspected too.
- WAD/MD3/TGA unit tests check mip creation, preservation of old directory entries/pixels, case-insensitive name collision rejection, truncated MD3/invalid indices and TGA RLE/truncation.
- Update fixtures check installation, settings/profile/MAP retention, preservation of a modified WAD using the distribution manifest, replacement of unmodified defaults, unsafe ZIP-path rejection and rollback after a forced partial-install failure. A separate disposable network check downloaded the real published alpha.29 package, verified its checksum and installed it over fake old binaries without executing it.
- Quake 1/LibreQuake: 18 integration assertions, packed MAP open/save/build, unchanged unrelated PAK entries and WAD bytes, starter/definition selection and model preview. Quake 3: 17 build/preview assertions and pixel-confirmed shader animation with a stationary camera (difference 11.52).
- Click/XY/Alt-Z/cancel/create/Undo/Redo gestures pass. A screenshot taken while a move was still active confirms the source brush is hidden. Workspace tests pass 14 assertions and restart persistence with unchanged MAP output.
- Embedded mod preview passes camera/player control, return-to-editing and restart persistence, with unchanged fixture progs.dat and MAP. The large MAP round-trip passes with 3,492 brushes, matching entity/face records and maximum plane drift 0.000557; the original file hash is unchanged.
- ImGui patch reverse-check and git whitespace checks pass. FTEQCC compiles without warnings. All tests operate on disposable installations; private maps and mod assets are not distributed.

Commands: `tests/alpha30-unit.cpp`, `tests/test-update.py`, `scripts/test-desktop-windows.ps1`, and the existing Q1 PAK, build-preview, gesture and workspace scripts. User interaction across display scaling/GPU setups, Explorer privileges and custom model definitions is covered by the [acceptance checklist](docs/alpha30.md).

## Alpha.22 verification, 2026-10-21

The background-build fixture passes 17 checks with real q3map2: BSP/VIS/LIGHT, compiled-world and brush-entity preview, lighting, editing restoration, LIGHT-only rebuild, rejected empty stages, failed/cancelled jobs retaining the previous result, and rejection of an outdated snapshot. Complete MAP exports match before/after preview, after Redo, and when saving directly during preview; Undo changes the export as expected. Actual engine screenshots were inspected.

The 5,015-brush MAP round-trip passes with unchanged original SHA256 and maximum plane-coordinate drift 0.000557. Mouse gestures and Undo/Redo pass. The action catalogue passes 343 actions without shortcut overlaps. FTEQCC compiles without warnings; the new render-world patch applies to the previous patched engine tree.

Run `scripts/test-build-preview-windows.ps1` for the disposable compiler fixture. Interactive acceptance checks and remaining limitations are in [the Alpha.22 guide](docs/alpha22.md).

## Alpha.21 verification, 2026-10-21

Windows x64/OpenGL, pinned FTE and Dear ImGui, MinGW native plugin and FTEQCC addon. The addon compiles without warnings. All 11 engine patches apply to the pinned revision; patched source hashes match the engine build tree.

- A disposable copy of the 5,015-brush regression map opens, supports Select All/Invert and saves/reopens with matching entity/face counts. Maximum plane-coordinate round-trip drift: 0.000557. The original's SHA256 remains unchanged. No private MAP or game assets are distributed.
- Linked groups: 16 checks cover duplicate/relative transforms, synchronization, nested plain groups inside linked ancestors, protected properties, transformed angle restoration, extraction and atomic Undo.
- Patches: 10 checks cover conversion, odd polygon decomposition, control-point edits and shape-preserving subdivision. Sweep: 10 checks cover straight/arc segments, seams, iterations, Undo and Escape reset.
- UV: 11 checks cover offsets/rotation/shear, singular-matrix rejection, geometry preservation, cancellation and command repetition. A native pointer drag additionally changes exactly one selected face; one Undo restores the complete original face records.
- Repeat: seven checks cover recorded sequences, ten successive duplicates, selection clearing and atomic Undo.
- Issues: 12 checks cover missing links, quotes, paths, fractional vertices, soft bounds, missing-mod removal, stale-result protection and Undo. Five shortcut/preferences checks pass, followed by a separate process restart confirming persisted settings.
- Compile/Launch: ten fixture checks cover export, external process output/failure/Stop, file copy/rename/delete, wildcard batches, variables, relative paths and source-map alias protection.
- A real NetRadiant-custom q3map2 2.5.17n-git-db756dc process ran through the editor's Export + Run Tool profile and produced a nonempty BSP from a disposable fixture. This verifies the compile integration, not lighting, VIS or gameplay on a production map. The final action catalogue test covers 342 static/dynamic actions without shortcut overlaps.
- Backup tests verify rotation, nonempty MAP copies and source manifests. Export tests verify OBJ exterior winding, UV/material records and tessellated patches; filter tests verify omitted layers and retained included group metadata. LIN/PRT validation and overlay navigation are exercised separately.
- Entity/property, clipboard, history, nested organization, affine/scale/vertex and full MAP regressions were rerun after the shared entity-string parser change. Actual engine screenshots were inspected for Preferences, Issues, UV, Sweep and run profiles.

Test scripts are `scripts/test-{linked,patches,sweep,uv,repeat,issues,run,backups,export,regressions21}-windows.ps1`, with `-Bundle` and `-Python`. `test-map-roundtrip-windows.ps1` also takes `-MapPath` and works only on a disposable copy. The original file is hashed before and after. Native tests run only with the disposable-test setup flag.

These checks cover the specified fixtures. Real compiler/game setup, Blender import, recovery-dialog interaction, unusual linked hierarchies, visual artifacts on other GPUs and arbitrary pointer sequences still require the [manual checklist](docs/checks-alpha21.md). Earlier sections below are historical records for previous releases.

The smoke test intentionally compares complete exported face records, including texture axes and optional metadata. Do not weaken it to ignore differing surface flags: older FTE binaries can emit uninitialized flags even when geometry looks correct. A binary is not qualified for this preview until the complete round-trip passes.

The screenshot check verifies actual FTE/ImGui rendering. It is not a substitute for clicking controls, cancelling dialogs and checking focus. The user checklist in README covers those remaining interactive checks.

## Recorded verification, 2026-10-19

- Native Windows x64 plugin compiled with MinGW GCC 15.2; csaddon compiled with pinned FTEQCC, zero warnings.
- Actual FTE/OpenGL screenshot confirms readable ImGui, grid floor and generated cube on NVIDIA RTX 4060 Laptop GPU.
- The full export regression failed on the old supplied engine and on the unpatched pinned engine. The pinned engine plus the included two-field copy patch passes: 12 faces after create, 6 after undo, and identical complete face records after redo, rejected invalid input and reload.
- Manual pointer/keyboard focus, dialog cancellation and Windows scaling checks remain for the first user test. No parity claim is made for advanced tools or Q2/Q3 document metadata.

## Version 0.2

Gesture regression: click leaves geometry unchanged; XY moved a 128-unit cube by (48,-16,0); Alt-Z then moved it by (0,0,32); cancellation preserved the exact export; drag created exactly one six-plane box; one Undo removed only that box and Redo restored it. The native frontend routes to these same QC functions. To reproduce, copy tests/gestures.cfg to the runtime game folder, run FTE with +exec gestures.cfg, then run python tests/check-gestures.py <runtime-game-folder>/maps. Real pointer routing, F1 and modifier transitions still need manual checks.

## Version 0.3

- `scripts/test-project-windows.ps1 -Bundle <bundle>` creates original loose and PK3 fixtures in a disposable installation. It validates 8/32-unit drag heights, nested archive paths without directory entries, a shader with qer_editorimage and no matching image name, material assignment in the export, and a one-second W press routed through the native MenuEvent handler while the inspector is open. Source fixture hashes remain unchanged.
- The complete create/undo/redo/invalid-dimensions/MAP-reload smoke test passes. QC compilation reports zero warnings. Engine screenshots verify the startup dialog and depth-tested surface grid.
- Manual checklist: load your real baseq3, filter and preview several materials, create brushes at grid 8/32/64, fly with WASD without RMB, type a material filter without moving the camera, and verify restart remembers the path but still asks for a game. Full retail Quake 3 resources and BSP compilation were not tested.

## Version 0.4 — 2026-10-20

- `scripts/test-editing-windows.ps1 -Bundle <bundle> -Python <python>` checks one-face material assignment, scales 2/4, a 90-degree rotation, unchanged other faces, complete Undo/Redo and MAP reload, whole-brush material application, two-half clipping, front/back retention and rejection of tangent, missing and collinear cuts. Half-brush bounds verify the split preserves the original box volume. Each accepted cut is undone as one transaction.
- Native event routing changes grid 16 → 32 → 16 with wheel events and activates Clip with C. Engine logs and screenshots verify the permanent workspace returns after opening another map. Actual FTE screenshots show material tiles, selected material, UV controls and colored cut previews.
- Basic smoke and gesture tests pass: click selection, XY and Alt-Z movement, cancellation, drag creation, separate history steps and full face-record round trips. Project fixtures pass 8/32-unit heights, nested PK3 and loose resources, shader discovery, WASD routing and read-only source hashes. QC compilation reports zero warnings.
- Regression fixes cover brush identity remapping through successive history operations, ownership of material strings in snapshots and drag previews, and a mixed integer/float QC condition that incorrectly treated creation as movement.
- User checks still required: real mouse selection of cut points and single faces, typing/committing UV fields, filtering real baseq3 collections, scrolling a large library, Windows scaling and focus during dialogs. Automated commands exercise the native event handler and shared QC editing functions; they do not replace those interactive checks.

## 0.5.0-alpha.1 / roadmap task 0.1 — 2026-10-20

PASS: optimized native build; action routing (190 entries, disjoint chords, native Ctrl+Z/Ctrl+Y/Ctrl+Shift+Z, text/popup guards, C, reserved/unknown actions); editing/material/UV/clip round trips; smoke; gestures; project PK3/loose assets and WASD. The action reference was inspected in an actual FTE screenshot (`screenshots/actions.png`). Screenshots must be captured with only one test instance running: background rendering can otherwise leave a stale frame.

Run `scripts/test-actions-windows.ps1 -Bundle <bundle> -Python <python>`. To regenerate the shortcut reference, run `python tests/check-actions.py <test-game-dir> docs/shortcuts.md`. Catalogue edits belong in `tests/generate-actions.py`; regenerate both outputs with that script.

Physical input, OS focus loss, non-Latin keyboard layout and Windows DPI are **pending user acceptance**, not claimed as automated coverage. Follow [user manual](docs/manual.md) before proceeding to 0.2. This version does not implement all reserved actions or change legacy wheel/camera behavior.

## 0.5.0-alpha.2 — tool lifecycle preview

Task 0.1 was accepted by the user on 2026-10-20. The 0.2 lifecycle test (`scripts/test-tools-windows.ps1`) drives native key events: C toggles, Escape removes three Clip points one by one, exits the mode and deselects; Ctrl+Z/Ctrl+Y restore/reapply deselection; V replaces Clip, and Shift+Escape clears synthetic handle state. The exported full MAP face records remain unchanged. Numeric Move by 32/0/0 verifies brush bounds and an exact Undo. This is a state test, not a claim of interactive vertex editing.

Manual acceptance remains [user manual](docs/manual.md), including physical drags, pointer hit testing of the new bar, modifier transitions and texture-lock behavior. Advanced geometry and its parameter pages remain later work; see the framework boundaries in that document.

PASS on the final QC/native build: tool lifecycle, action routing, gesture constraints, editing/UV/Clip round trips, smoke and project resource/WASD tests. QCC reports zero warnings. Actual FTE screenshot: `screenshots/tools.png`.

## Version 0.5.0-alpha.16 — 2026-10-20

The batch includes selection commands, box creation, mixed-object transforms, clipboard, material tools, Rotate and Extrude/Split/Stamp. Tests ran in disposable installations with the pinned engine and all eight patches. QC compiled with zero warnings; the native plugin compiled successfully.

Passing focused checks: 17 selection-command checks, 11 shape checks, 20 mixed-transform checks with complete MAP comparisons, 6 surface-metadata checks, independent group/model copies, clipboard parser/export/native-QC transfer, 17 material checks, 9 texture-transfer checks, 15 affine checks, 7 interactive Rotate checks and 20 Extrude/Split/Stamp checks. Extrude covers shared opposing faces, collapse limits, world-space UV, 2D gestures, split direction, prism volume and perpendicular sides on a sloped face.

Regression checks passed for 38 face-selection cases, 52 object-selection cases, 26 geometry cases, 37 grid cases, gesture constraints, 27 view/Clip assertions and 12 visual-state checks with complete unchanged MAP data. The old 2D creation assertions were updated for outward snapping and camera-side default thickness introduced in task 3.1; they no longer expect nearest-rounded endpoints.

Actual engine images were inspected for the alpha.16 Split preview and material inspector, including alignment buttons and attribute controls. The clipboard transport test uses an isolated fixture and preserves the OS clipboard; actual application-to-application paste remains in the manual checklist. The full roadmap is not complete. See [features](docs/alpha16.md) and [combined checks](docs/checks-alpha16.md).

## Alpha.17 workspace

Run `scripts/test-workspace-windows.ps1 -Bundle <bundle> -Python <python>` in addition to the layout/action/material/transform/extrude suites. The workspace runner uses two separate engine processes and a disposable installation: 14 assertions cover default docking, splitter drag, central scene input, floating panel input, rasterized font scaling, local persistence, restart and reset. It compares complete before/after MAP bytes and verifies screenshots contain the intended panel colors. Inspect `workspace-dark.png`, `workspace-floating.png`, `workspace-large.png` and the material screenshot visually. See [manual checks](docs/alpha17.md).

## Alpha.18: convex editing, entities and organization

All tests use disposable installations. The native plugin and QC compile successfully; QC reports zero warnings. The engine includes the previous eight patches and `editor-fog-toggle.patch` (OpenGL global and local-volume fog control).

Focused passing suites: Clip interaction (16), CSG (13), Vertex (28), Rotate (10), Scale/Shear (13), Assemble (11), primitives (16), entity properties/ownership/defaults (24), groups/layers (17), filters/tags/links/export (19 plus native destination validation). The entity definition parser fixtures cover ENT/DEF/FGD and inheritance. Native property-input tests cover Tab commit, Ctrl+Enter creation, rename, value editing and Undo. Image comparisons cover patch textured/flat/shaded/wire modes and fog on/off; the MAP remains byte-identical.

Regression suites include full document MAP round-trips (brush/entity/patch), Unicode file browsing, clipboard transport, object selection (52), selection commands (17), materials (17), texture transfer (9), affine (15), Extrude (20), editing/UV/Clip history, camera input (41), rebindable action catalogue and workspace persistence (14). Run `scripts/test-<suite>-windows.ps1` with `-Bundle` and `-Python`; dedicated new runners are listed under `scripts/`.

Actual engine screenshots were inspected for the entity property table and scene render modes. Automated coverage does not replace manual tests of real baseq3 resources, mouse ergonomics, drivers or DPI. [Complete checks](docs/checks-alpha18.md). No linked groups, patch editing tools, Sweep or UV editor were implemented.

## Alpha.19 — 2026-10-21

Native plugin and pinned engine rebuilt successfully; QC compiles with zero warnings. All ten engine patches apply in order to the pinned source and reproduce the files used for the engine build. The action generator reproduces the static catalogue; definition-derived class/create actions extend it at runtime (325 actions with the bundled definitions).

Focused passes: entity properties (28), extended native property-table input (Tab, Ctrl+Enter, rename, Enter, arrows, F2, Delete routing and Undo), Vertex (29, including UV coordinates on newly triangulated faces), organization (19, including one-step batch Undo), filters (19), unselected submodels (10 plus image/MAP comparisons), Assemble (11), primitives (16) and the native ENT/DEF/FGD parser fixtures. Actual engine screenshots were inspected for the property table and unselected group/brush-entity surfaces.

Regression passes include geometry (26), Clip interaction (16), CSG (13), Scale (13), Rotate (10), affine (15), materials (17), clipboard transfer, complete document round-trip, object selection (52), selection commands (17), external Unicode file browsing, render modes/fog, workspace persistence (14) and transaction history. The history fixture now respects phase 8: Delete belongs to the last Clip point; with no point it leaves geometry and history unchanged. The document test compares oriented plane equations instead of arbitrary point bases while retaining exact texture/surface attributes and entity/patch checks.

`test-map-roundtrip-windows.ps1` copies its input into a disposable installation, checks the original hash, saves/reloads/saves, compares plane counts and equations, then exercises Select All and Invert. An independently generated 16-brush thin-bevel fixture passes. The private `hotel02.map` test retains 2394 brushes and 2 patches through repeated saves, with maximum plane-distance drift below 0.001 units; all 2396 brush/patch objects can be selected and inverted without QuakeC runaway. The user's map and game assets are not distributed. FTE still applies its normal import tolerances to nearly degenerate source faces; this is not a byte-identical source-format promise.

The geometry benchmark measured approximately 6 ms for a 64-plane prism and 26 ms for 500 boxes on the development machine. The native-geometry proposal remains the follow-up allowed by task 0.7. Physical input ergonomics, real material libraries and Windows DPI remain manual acceptance items: [30 checks in English](docs/checks-alpha19.md). Phases 16–20 were not implemented.

## Alpha.20 — 2026-10-21

The native plugin compiles successfully; QC compiles with zero warnings. The pinned engine and its ten patches are unchanged. Tests run in disposable installations without modifying game files.

Focused regressions pass: point and brush-entity Delete/Undo through native key routing (5 assertions), 32-sided cone apex reconstruction/commit/Undo and side-view/Alt gestures (8 assertions), single-click folder navigation with Unicode Open/Save As, and submodel visibility with surface-grid spacing/visibility and edge-toggle image comparisons. Actual engine screenshots were inspected for point-entity boxes/labels, group surfaces, grid/edges and the large-map scene. Render-mode and fog comparisons retain an unchanged MAP.

Regression passes: geometry (26), Vertex (29), primitives (16), CSG (13), filters (19), views/Clip (27), grid (37), entities (28), action catalogue/routing (325), file-browser full object round-trip and submodels (10 plus image checks). View/grid test readers now inspect actual axis-aligned planes instead of treating arbitrary plane point bases as brush corners. The private large map retains 2394 brushes and 2 patches through save/reload/save; maximum plane-distance drift is 0.000622 units and its original hash is unchanged.

At 1280x800 on an RTX 4060 Laptop, the same fixed large-map view measured 26.4 FPS in alpha.19 and 52.0 FPS in alpha.20 with visible surface grid and white edges. No retail texture library was mounted for this comparison. This is a local scene measurement, not a universal speed guarantee or a benchmark against TrenchBroom. Use `scripts/benchmark-map-windows.ps1` to repeat the measurement in a disposable copy. User maps and assets are not included in the release.

Final regression logs contain no repeated VM errors, modulo-by-zero or edit queue overflow. The engine may still report its optional missing server addon once when loading an editor map. MAP-line diagnostics now require explicit debug mode. Manual acceptance: [English](docs/checks-alpha20.md).

## Alpha 24 regression evidence (2026-10-22)

- Native ImGui Scale X/Y typing: 0.375 and 1.75 committed through the real command dispatcher.
- 20 Extrude checks; 10 silhouette checks (direct/hidden/far/XY/shared seam/disconnected coplanar faces).
- 500 selected cubes: warm silhouette picking averaged 1.76 ms over 50 changing cursor positions on the test machine.
- 52 selection checks and byte-identical selection MAP round-trip; native gesture checks for XY/Alt-Z/create/cancel/Undo/Redo.
- `hotel02.map`: 2,394 brushes and 2 patches preserved through two saves; maximum plane drift 0.000533 units. Original SHA-256 unchanged: `1B5809E52B1BBAD39D77174047E74D0EEA46491FD68DFD6FA67293DF0B7C6ABE`.
- Actual external hotel02 document exported and q3map2 BSP stage completed: 3,038,324-byte BSP, approximately 7 seconds. The original was never a compiler output target.
- 17 background-build assertions including full fixture build, BSP/edit switching, LIGHT-only, failure/cancel retention and unchanged Undo/Redo. Stationary-camera animated shader pixel difference: 34.50.
- Two restarts restored baseq3 without setup. Untitled documents resolved baseq3/maps; the external hotel02 document resolved its own directory.
- Translucent shader fixture kept alpha on selected faces; inspected the actual engine screenshot.
- Extracted actual patched C functions tested for flat/curved/NaN/zero-tolerance patches, missing tangent basis and empty/nonempty shader text (`tests/check-engine-safety.py`).
- Controlled hotel02 viewport benchmark: approximately 29 → 72 FPS at 1280×800, same fixed camera and assets. This is not a general FPS guarantee.

User-map source paths are read-only test inputs. All automated writes use disposable installations or exported snapshots.

## When the sky went dark profile (alpha.25)

Run `scripts/test-mod-profile-windows.ps1` with `-Bundle`, `-RuntimeMod`,
`-RuntimeEngine`, and optionally `-Python`. It copies the runtime mod into a new
temporary directory (requires enough free disk space), builds a sealed fixture,
launches and stops a separate read-only FTE runtime, checks source/progs hashes
and profile persistence, and captures the settings panel. No QC editor code is
changed by this release. Runtime appearance and entity behavior still need the
manual checks in `docs/when-the-sky-went-dark.md`.

Verified locally: seven profile/persistence assertions; BSP/VIS/LIGHT fixture
compilation; 86 definitions imported from the local mod's QUAKED comments; actual
runtime window showing the fixture and the mod's player models. These assets and
local generated definitions are not included in the release.

## Quake 1 (alpha.28)

Verified on Windows x64 with the packaged FTE renderer and ericw-tools 0.18.1:

- Unit checks for malformed WAD offsets, palettes, BSP29/BSP2/IBSP validation, standard/Valve 220 UV conversion, patch rejection and entity definitions.
- Actual QBSP/VIS/LIGHT processes on an original WAD/palette fixture, BSP29 and BSP2 output, LIGHT-only rebuild, BSP/edit switching and standard MAP save. Inspected the rendered palette and lightmaps in an engine screenshot.
- Embedded Quake runtime: acknowledged free camera, player camera, input release, return to editing and another LIGHT/full build. Game resources were copied into a disposable installation.
- Quake 3 regression: 17 build assertions, cancellation/failure retention, unchanged document/Undo/Redo and animated shader frames (pixel difference 32.23).
- When the sky went dark: existing definitions/profile persistence, build, embedded camera and restoration, unchanged source document and progs.dat.
- Gesture regression: selection, XY move, Alt-Z move, cancel, creation and Undo/Redo.
- Large MAP round-trip: 3,492 brushes, same entities/faces after two saves, maximum plane drift 0.000557 units; original input hash unchanged.

Run `scripts/test-quake1-windows.ps1 -Bundle <bundle> -Python <python>`;
add `-RuntimeMod <game-directory>` to include play/free-camera testing on a copy.
The compiler runtime prerequisite is documented in `docs/quake1.md`.
Manual acceptance, including real-game entities, liquids, sky and animated WAD
textures, remains in `docs/quake1.md`.

## Quake 1 resources / PAK / model browser (alpha.29)

- Downloaded the exact LibreQuake v0.09-beta Mod Lite URL: 55,980,564 bytes; pinned SHA-256 verified. Extracted in a disposable installation; original game folders were not modified.
- PAK unit checks: entry replacement, unchanged binary payload, backup, directory navigation and traversal rejection. Parsed the actual LibreQuake Ogre MDL (556 triangles), including truncated-input checks.
- Mounted-PAK integration: 18 assertions covering MAP open/save/build, automatic packed WAD discovery, Quake starter, Quake/LibreQuake definitions, rendered model thumbnail, free/play cameras and browser navigation. Verified unchanged non-MAP archive entries against the backup.
- Windows saves temporarily detach authoring/mod mounts before atomic PAK replacement, then restore mounts on success or failure. The engine patch prevents memory-mapped PAK handles. The regression deliberately saves into a mounted id1/pak2.pak.
- Inspected the actual engine screenshot: textured Ogre appears in the entity browser.
- Quake 1 regression: QBSP/VIS/LIGHT, BSP29/BSP2, LIGHT-only, preview/return and standard MAP save.
- Quake 3 regression: full build, BSP/edit switching, cancellation/failure retention, document/Undo/Redo preservation and dynamic shader frames (pixel difference 8.76 with a stationary camera).

Run `scripts/test-quake1-pak-windows.ps1 -Bundle <bundle> -LibreInstall <download-directory> -Python <python>`. The download directory must contain `ready.txt` and `mod_lite/lq1`; the test copies it, never edits the original. Manual download/error/retry, profile-switch and archive editing checks are in `docs/alpha29.md`.
- When the sky went dark: repeat run passed embedded free/player camera, player movement, input release, restoration, restart persistence and unchanged document/progs.dat. The first timed run did not observe two camera trace messages; no code changed between runs. Manual camera switching remains in the checklist.

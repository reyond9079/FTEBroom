# Native models and Half-Life fixes — alpha 95

## Half-Life resources and preview

Select the game's `valve` folder in **Edit → Preferences → Game profile**.
Choose FreeHL for Half-Life or FreeCS for Counter-Strike 1.5. Compile the current
saved map, then choose **Preview mod**. Windows embeds the game; Linux launches
the runtime in a separate window. Native GoldSrc DLL mods are not executed by
these open implementations.

Runtime `.def` declarations in `decls/def` no longer replace editor entity
definitions. Automatic discovery checks for an editor-definition signature;
the bundled FGD remains the fallback. Explicit definition overrides still win.
Pending map opens are protected from automatic start-scene replacement during
WAD reindexing.

The preview stages its matching runtime modules and defaults in a private home
beside the build. Loose legacy `progs.dat`, `csprogs.dat`, and `fte.cfg` files in
the game directory cannot override this private runtime. Linux also launches a
private native-renderer engine copy (also in the SDL2 edition), keeping the
editor installation manifest out of the game
preview. The game launcher menu is bypassed; the requested map starts directly.
Existing game modules,
configurations and WADs are not rewritten. The separately named editor runtime
package is still installed as described in the [game guide](legacy-games-alpha94.md).

**Show BSP** initializes the compiled world with fullbright disabled. Returning
to MAP editing restores the previous lighting setting. Generated WAD/WAL aliases
also retain their lightmap stage. Missing Half-Life `_light` keys are initialized
on new lights and normalized
from legacy scalar `light` values in saved/compiler maps; explicitly authored
`_light` values are retained. Enable the LIGHT/RAD stage under **Setup
compile**; disabling it cannot produce baked illumination.

## Native Quake II MD2

Open a model in **Model editor**, adjust it, then use **File → Export model as…**
and choose **Quake II MD2**. Quake II imports default to MD2.

MD2-to-MD2 export keeps all original frames, frame names, UVs and skin variants,
regardless of the sampling controls. Original PCX skins are copied without colour
conversion. Exported skin paths point to the new model's skin directory. Model
position and units are retained; transformed positions and normals are encoded
at MD2's native precision. Move the PCX directory together with the model.

Other source formats bake the selected animation into MD2 and a padded PCX
atlas using the active Quake II palette. UV wrapping is baked into each tile.
This path uses 256-pixel material tiles and palette quantization; it cannot keep
modern shader effects or a skeletal rig. Limits: 2,048 vertices, 4,096 triangles,
512 frames and the format's 64-byte skin paths. Geometry is not automatically
resized, recentered or simplified.

## Native GoldSrc studio MDL

Half-Life imports default to **GoldSrc MDL**. Existing studio MDL v10 models can
be moved, rotated and scaled as a complete rig in Model editor. Enable **View
bones** to inspect the skeleton. Bounding boxes include the visual header box
with sequence boxes as a fallback, so the camera can frame models with an empty clipping box.

Studio-to-studio export retains every sequence and blend, animation timing,
activities, events, bodygroups, skin families, embedded texture palettes,
hitboxes, attachments and bone controllers. Required external animation groups
and texture companions are read and embedded in the new MDL. Keep companions
beside the source while exporting; they are no longer needed by the exported
standalone model. Native export ignores the single-sequence sampling controls.

Use **Total scale** for uniform scaling. Rotation/translation are applied by a
constant parent bone, preserving the original root-motion axes and controllers.
This needs one spare bone beneath GoldSrc's 128-bone limit. Nonuniform scaling
of an animated rigid skeleton can introduce shear that GoldSrc cannot represent;
it is rejected instead of silently distorting the animation. Animation channel
precision and 16-bit relative offsets remain GoldSrc format constraints.

Other model formats can be baked to studio MDL. Static meshes use a root bone;
vertex animation uses independent motion tracks and is limited to 128 vertices
by GoldSrc's bone limit. Larger vertex animations must use MD2/MD3 or an existing
skeletal studio source. Generic studio baking supports up to 2,048 vertices,
20,000 triangles, 32 materials and 512 frames. Textures must have dimensions
divisible by four and be at most 512×512; unsupported dimensions are rejected.
Textures with at most 256 RGB colours retain them; others use RGB332 conversion.
Transparency and modern shader effects are not retained by generic baking.

Exports require a new filename. Source models and original textures remain intact.

## Reproducing the native format checks

Build `tests/model-native-export.cpp` with a C++17 compiler and pass a disposable
output directory. It checks native headers, skin/atlas encoding, coordinate
preservation across poses, transforms, studio reference geometry, animation
retention, and rejection of truncated or unrepresentable input. The generated
fixtures can then be opened in Model editor; play each animation and inspect
skins, bones, position and dimensions.

Retail game assets used in local verification are never included in the source
repository or release archives.

## Local verification

Windows checks used the installed Half-Life `valve` resources and a disposable
six-brush room, not modified retail maps. The normalized light produced 6,024
bytes of varying RGB lightmaps instead of a one-byte empty lighting lump.
The client reached `ftebroom_preview_ready` and accepted the editor camera.
Native export roundtrips were reopened textured and animated in Model editor.

All 128 sequences of `scientist.mdl` (377 sampled blend/pose combinations) and
32 sequences of `agrunt.mdl` (105 combinations) were compared independently
before/after a 1.5x scale, XYZ rotation and translation. Maximum bone-matrix
component error was below 0.001. Original texture palettes were retained; the
external texture companion was embedded. These assets are not redistributed.

The portable format tests passed on Windows and Linux. GUI/compiler/runtime
checks were also run in an Arch Linux WSL test environment using WSLg Wayland
and software Mesa; this does not constitute a test on physical Linux graphics
hardware or a complete KDE desktop session.

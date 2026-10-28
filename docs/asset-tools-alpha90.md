# BSP model export, WAD creation and palette inspection

## Export selected brushes as a BSP model

Choose whole brushes, then **File > Export selected BSP model...**. Enter a new loose `.bsp` filename or use **Choose folder...**, and click **Compile selected brushes**. The original MAP does not need to be saved first. The source document, its selection, Undo history and existing destination files are preserved.

The exporter creates a separate brush-only MAP snapshot. Point entities and unselected brushes are excluded. Selected brush-entity geometry becomes ordinary model geometry; entity behavior is not exported. Patch selections are rejected rather than silently approximated. Brush position and size stay in their original world coordinates; the model is not recentered.

Quake 1 uses the configured/bundled ericw QBSP with `-nofill` and the profile's BSP29/BSP2 choice. Collision hulls remain enabled. Quake III and FTE Custom use Q3Map2 geometry compilation with an exterior flood seed in the temporary snapshot. The seed is removed from the published BSP. Open geometry is allowed; leak diagnostics from Q3Map2 are expected for this export. Neither VIS nor LIGHT runs. This is a brush model, not a playable level. Configure compiler paths in **Setup compile**.

Compiler output is available in **Compile / Launch**. Export errors are also displayed in the export window. Cancelling compilation publishes no new model. Temporary inputs and `compile.log` are retained under the editor's `data/bsp-model-export` directory for diagnosis.

### Atlas and painting template

Enable **Create texture atlas and UV painting template** to bake the existing diffuse appearance into independent UV islands. Each compiled face receives its own island, including repeated texture coverage. UV coordinates in the BSP are actually reassigned; this is not just a contact sheet of source textures. Position, polygon shape and collision stay unchanged.

The sibling `<model-name>-atlas` directory contains:

- `atlas0.tga`, `atlas1.tga`, etc.: baked diffuse color pages.
- `uv0.tga`, `uv1.tga`, etc.: matching transparent UV boundary overlays for painting.
- `PAINTING.txt`: format-specific paths and instructions.

Open an atlas in an image editor and place its corresponding UV image on an overlay layer. Keep the image dimensions and island positions unchanged. Quake III BSP references these game-relative TGA files directly: keep the atlas folder at its exported location and reload resources after painting. For Quake 1, the BSP embeds indexed atlas textures. `PAINTING.txt` names each embedded texture and the FTE `textures/<name>.tga` replacement path. For an engine without external replacements, use a BSP texture tool to re-embed painted textures. Editing a sibling TGA alone does not rewrite a Quake BSP.

Atlases support static opaque diffuse textures. Animated shaders, sky, liquids and masked textures cannot be faithfully flattened; export without the atlas to preserve them. Q3 shader previews provide the static editor/first-stage image, not a bake of runtime GLSL or animation. A missing source image stops atlas export. Quake 1 retains original palette indices, including fullbright colors, in the embedded base mip.

Pages are 1024×1024 with eight-pixel padding and mip-aligned packing. The limit is 16 pages / 4096 compiled faces / 32 million source image pixels. A face spanning more than 1006 texture pixels stops export instead of silently reducing texture resolution. Choose a smaller selection or export without an atlas. These are explicit atlas limits, not a limit on ordinary BSP compilation.

## Create WAD2 archives

In the image import window, choose **New WAD...**, enter a filename and create the archive. It becomes the selected destination for the queued images. The existing resize, Quake palette conversion and mip generation still apply when importing the images.

In **Mod file manager**, right-click the directory background (or a folder) and choose **New WAD...**. This creates an empty, valid Quake WAD2. Existing files are never replaced. Creation inside writable PAK/PK3 folders uses the file manager's backup-preserving archive writer. Importing images directly into a WAD nested in another archive remains unsupported: use a loose WAD for that workflow.

## Palette viewer

Double-click `gfx/palette.lmp` inside a PAK/PK3 or as a loose file, or use **Open palette viewer** in its context menu. The viewer reads the virtual archive member directly and displays all 256 RGB colors. Hover a swatch for its index, RGB and hexadecimal color; click to keep it selected.

The viewer is read-only. A Quake palette is exactly 768 bytes. Other LMP files can contain pictures or unrelated data; they receive an explanatory message rather than being interpreted as a palette. Index 255 is transparent only when the material uses masked transparency.

## Checks

- Export one brush while a second brush and a point entity are unselected. Only the first brush must appear in the BSP.
- Repeat for a selected group, Quake BSP29/BSP2, Quake III and FTE Custom. Confirm original position/size and collision.
- Export open geometry. No sealing box should be required. Check the compiler log and errors for an empty selection or a selected patch.
- Export with the atlas; inspect both color and UV images, paint a visible mark, and follow `PAINTING.txt` to load the painted texture.
- Export without an atlas to preserve special materials. Try an existing destination filename: it must not be overwritten.
- Create a WAD during image import, import an image, paint a brush with it and compile. Create another WAD from the file manager context menu.
- Open a palette LMP inside a PAK and check swatch indices. Open a non-palette LMP and check its error message.

## Validation performed

- Windows: real editor exports in Quake 1 and Quake III, with an unselected brush and point entity present. Published BSP bounds contain only the selected brush; the synthetic Q3 entity is absent.
- Arch Linux / KDE Plasma 6.8.1 / Wayland testbed: Quake 1 export with atlases, WAD creation, overwrite rejection and packed palette reading in both native and SDL2 builds. The testbed uses software rendering; this is not a physical GPU/input certification.
- Windows and Linux compiler integration tests: actual ericw BSP29/BSP2 and Q3Map2 output; unchanged position/collision lumps; reassigned UV sampling against original texture data; visible UV boundary images; corrupt-offset rejection. The Quake palette fixture includes duplicate RGB colors at ordinary and fullbright indices to check that indices are retained.

The repeatable compiler test is `tests/test-bsp-model.py`, using the `tests/bsp-atlas-unit.cpp` validator. It creates its own temporary MAP, WAD and texture resources and does not read or modify user maps.

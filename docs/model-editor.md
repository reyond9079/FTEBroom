# Model editor

In the Mod file manager, right-click a model and choose **Open in model editor**.
The preview is on the left and the inspector is on the right. Drag the divider
to change their widths. This editor belongs to the ordinary desktop version;
it does not require an online server.

## Transform a model

Select **Model (root)** to edit the entire model, or select a mesh in the hierarchy.
Choose Move, Rotate or Scale and drag the coloured handles in the preview.
Alternatively, enter values in the inspector:

- Position XYZ is the selected pivot's world position.
- Rotation XYZ and Size XYZ use the selected node's local axes.
- Scale XYZ changes individual axes. Total scale multiplies all three axes.
  Both are numeric fields, with no sliders.

Mesh pivots initially use their bounds centre; the root pivot is the model's
original origin. The gold marker highlights world zero. Long RGB axes cross
world zero in both directions; the floor grid is at Z=0. The solid gizmo edits
the selection.

The Parent selector builds an editing hierarchy. Moving, rotating or scaling a
parent affects its children. Changing a parent retains local transforms, so it
can change the mesh's world position. This hierarchy groups editable surfaces;
it is not a skeleton editor and does not reconstruct an authoring application's
original object hierarchy.

## Inspect

Use the triangle Play button to start animation and the Pause button to stop.
The existing sequence, frame and skin controls remain available.

The View menu offers Textured, Untextured, Wireframe, View bones, Floor grid,
Origin axes and Backface culling. Bones are drawn over the model. Textured mode shows
diffuse textures with neutral lighting; it does not reproduce every runtime
shader effect.

**View from player perspective** uses a static camera at the model origin,
looking along positive X with a 90-degree horizontal field of view. It starts
enabled for filenames beginning with `v_`. Disable it to orbit or fly.

Hover the preview and use WASD/QX to fly; hold RMB to look around. Shift speeds up movement; Alt slows it.
MMB pans; the wheel zooms or moves the free camera. Release the mouse button
or press Escape to release the cursor. View > Frame model / reset camera
restores the framed orbit view. Camera preferences match the map editor.

## Export

Choose **File > Export model as...**, select a destination folder and format,
review the warnings, and export. The original model remains unchanged.
Transforms are baked into the exported geometry, without automatic recentering
or normalisation. Exported textures accompany the model when required.

Available writers are MD3, Quake MDL, OBJ, IQM and MD5MESH. FTE can read other
formats for which this editor does not provide a writer. MD3 and MDL support
baked vertex animation. OBJ, IQM and MD5MESH export a static pose; original rigs,
tags and animation events are not preserved by these writers. Cross-format
conversion can change material layout and coordinate precision.

Direct Quake MDL-to-MDL export preserves its embedded skins, UVs, triangle
layout and frame-group timing while transforming all frames. MDL still uses
its native quantised coordinates. Simple external `.skin` surface mappings
are supported; composite skin directives require a different export path.

## Check after updating

1. Open a textured animated model from the file manager's context menu.
2. Confirm the inspector is on the right; resize its divider and window.
3. Enter XYZ position, rotation, size and scale; change Total scale.
4. Drag each move axis and rotation ring. Reset selected transforms.
5. Parent one mesh to another and check parent transforms affect both.
6. Play/pause animation; change skins and enable the bones overlay.
7. Switch all display modes and check world origin/reference axes.
8. Open a `v_` model; toggle player perspective and orbit mode.
9. Export a copy, reopen it and check textures, dimensions, origin and animation
   where supported. Check the original file is unchanged.

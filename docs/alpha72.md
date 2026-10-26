# ReyondBroom alpha 72

- Native plugins with a versioned C/C++ SDK, Plugins manager, independent
  Windows/Linux libraries, enable/disable, rescan and load-error reporting.
- Bundled Crumble plugin combines Perforate, Fracture and asynchronous Settle.
  It releases fragments outside the wall before falling by default, with
  adjustable clearance/direction. [Plugin guide and SDK](plugins.md).

- Quake 1 player-model recovery after mounting resources no longer depends on
  a notification sent before the map's editor VM is ready. Packaged engines
  must support reloading asset models, including failed startup loads.
- The yellow/orange map-boundary frame and sky surface overlays default to off. Quake 1 `sky*` names are recognised
  before the material-tag cache is available. View options retain the toggle.
- Double-clicking models in the file tree and tile/list view opens Model editor.
  The separate Model viewer entry is retired.
- Model editor has a world-space floor grid and RGB axes extending in both
  directions through world zero. View > Floor grid and Origin axes toggle them.
  Grid density is bounded as the camera moves away; lines are depth-tested.
- Hover the preview to use WASD/QX, and hold RMB to look around. Shift doubles
  speed; Alt halves it. MMB pans. Map camera preferences apply. Release the
  mouse button or press Escape to release the cursor. Disable player
  perspective to fly. The map camera and document remain unchanged.
- View > Frame model / reset camera restores framed orbit. LMB orbits in framed
  mode; after flying it turns the camera at its current position.
- Linux packaging normalises shell launchers to LF before hashing/archiving.

## Verify

1. Select Quake 1 with LibreQuake. Check `info_player_start` in the starter scene
   and newly placed starts. Close/reopen the editor and reload resources;
   verify the models still appear.
2. Open a map with `sky*` textures. Sky overlays should be off initially;
   explicitly enable/disable them through View options.
3. Double-click models in the tree and tile/list view. Both should open Model
   editor with the transform inspector on the right.
4. Check the floor at Z=0, RGB axes in both directions and the gold origin
   marker. Toggle guides; move far away and return.
5. Hold RMB, look around and use WASD/QX, Shift and Alt. Release RMB and check
   the cursor returns. Check MMB pan, wheel, Escape and application focus loss.
   Return via View > Frame model / reset camera.
6. Enter transform values, use gizmos, play animation and export a copy.
   Camera/grid operations must not change exported geometry.
7. On Linux, start `Launch-ReyondBroom.sh` directly. Repeat the model checks.
8. Select one wall brush and open Plugins > Crumble. Apply default settings.
   Check holes, falling fragments and retained selection. Undo twice and Redo
   twice. Try an impossible diameter/count and check the source stays intact.
9. Open Plugins > Manage plugins. Disable/enable and rescan the idle plugin;
   confirm load errors are visible for incompatible or missing libraries.

Mod archives used for testing are copied to disposable directories. Original
game files, maps and models are not rewritten.

# Alpha 87: retained group edits and selection-guide safety

## Failure and fixes

The reported E1M1 selection failure was a VM runaway in
`fb_visual_spike_trace -> fb_render_allowed`. Selection guide rays could perform
24 script-side scans of a large brush collection, including temporary face
allocations. A native batch now resolves brush references once, rejects bounds,
and intersects the same convex planes. Selected, hidden and locked-object rules
are preserved. The existing script implementation remains the fallback.

Group Duplicate and Move previously failed the worldspawn-only incremental test.
The document then discarded all face/grid caches and displayed large-map
preparation again. Retained updates now accept brushes belonging to groups and
brush entities, refresh affected entity records and resolve owner/group links.
Container bounds, material counts, visibility and Undo/Redo IDs are updated.

The transaction is validated before retained slots change. Unsupported patches,
changes to an existing entity's owner type/model, capacity growth, more than
4,096 history records or more than 2,048 retained brush edits keep the full-loader
fallback. This is not a guarantee that arbitrary document-sized edits are instant.

Additional fixes:

- Index submodel owners and MAP source-line references rather than scanning the
  document once per brush. Grow copied source-line entries geometrically.
- Use the brush-node index when Undo/Redo remaps regenerated brush IDs.
- Join entity selection keys once, avoiding quadratic temporary strings.
- Key inspector publication by cached selection and entity revisions, avoiding
  rebuilding an enormous signature every frame.
- Submit selection-box lines natively, including a batch for large point
  selections. The box edges, colors, opacity and depth behavior are retained.
- Raise the compiler string-table capacity for the larger program; the VM arena
  and runaway protection are unchanged.

## CLIP texture order

A MAP face stores a material name, not the WAD that originally supplied it.
TrenchBroom mounts WADs in document order and searches mounts in reverse order;
its material manager also replaces an earlier duplicate name with a later one.
The alpha 86 document-order fix follows that precedence and remains included.

The two available E1M1 files have different WAD headers. With the actual available
archives, the original E1M1 FTE.map resolves `clip` to tools.wad, whereas the
rewritten E1M1 FTE (2).map resolves it to MG3Tex.wad. The actual archive audit
passes for the original header. Replacing an image without knowing the intended
archive would be a guess; this release does not rewrite either original map or
change its WAD order automatically. Material source information can be checked
in the material browser.

## Validation

Validation uses isolated copies of the maps and actual available WADs. Original
maps, WADs and mod files remain untouched.

- E1M1: Duplicate a 42-brush group, move the copy, Undo twice and Redo twice;
  assert that the base face/grid geometry epoch remains unchanged. Representative
  Windows runs on the Core Ultra 7 / RTX 4060 Laptop test machine took about
  0.35–0.40 seconds for Duplicate and 0.28–0.34 seconds for Move, including the
  document update, without full large-map preparation. These are operation times,
  not a claim of instantaneous editing or a new FPS benchmark.
- Select the group of 22,880 point entities and exercise all 24 collision guides
  (approximately 11 ms for the corrected guide batch in the final large-map run). The complete
  selection overlay for a group this large remains expensive; it is not a 60 FPS
  guarantee. No CSQC runaway occurred in the final large-map runs.
- Compare native guide intersections with the script reference on a small map.
- Check small-map group selection, mixed geometry/entity copy and Undo/Redo.
- Build Windows, Linux native and Linux SDL2 from the same QC and engine patch.
- Check Linux native/SDL2 under virtual Arch/KWin. This is a software-rendered
  lifecycle test, not certification of physical Linux GPU/input performance.

The retained rendering from alpha 85 is included. Historical alpha 85 FPS figures
are not new alpha 87 measurements.

## User checks

1. On E1M1, Duplicate the previously problematic group and move it several times.
   Check that no complete large-map preparation appears; check Undo and Redo.
2. Select entities in the upper plane, hover the selected group, and move the
   pointer/camera. Check that the viewport remains alive and guide rays stop at
   brushes with red contact dots.
3. Check group bounds after Move, Duplicate, Undo and Redo, including nested groups.
4. Compare a small map's entity/brush outlines and selection colors with alpha 86.
5. For CLIP, compare the same MAP and WAD list, and note the source archive shown
   by the material browser. Different MAP headers can legitimately pick a different
   texture with the same name.

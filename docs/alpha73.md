# ReyondBroom alpha 73

## Crumble fixes

- Retained Perforate plugs and Fracture's optional brush entities now receive
  their initial entity fields before key edits. Previously the empty reserved
  entity index was not yet part of the map; fragments existed in geometry and
  physics but were absent from the render/document tree.
- Auto through-axis prefers a feasible thin section. Horizontal slabs cut along
  Z; automatic release moves their plugs below the slab. Vertical walls release
  toward the camera. Explicit axes and manual release side remain available.
- Voronoi clipping uses coordinates around the source centre, avoiding lost
  edge matches on thin elevated brushes. Exterior materials/UV attributes are
  restored in world coordinates.
- A preflight checks projected planes, positive through-thickness, enclosing
  minimum-size clearance circles, count/spacing bounds and a fragment-volume
  bound before seed search. Constructive layouts certify some requests; an
  inconclusive search is not called mathematical impossibility.
- Geometry searches at most 16 seeds, with a certified minimum-size layout
  fallback when available. Physics searches at most eight seeds. Geometry
  failures roll back; changes to the document/selection stop pending work.
- Moving/falling results are not automatically applied. A supporting floor
  must exist below the released fragments; the viewport grid is not a floor.
- Applied fragments remain selected and are grouped under **Crumble fragments**.
  Their `func_wall` plug entities are preserved; the perforated source is outside
  the group. Grouping and falling share one Undo step.
- Completion reports moved/dropped counts, mean drop and the geometry seed.
  No-drop results are explicitly identified. Automatic framing and a separate
  **Frame fragments** button make the result easier to find.

See [the plugin guide](plugins.md) for exact limits and SDK details. The SDK ABI
remains v1. Crumble applies the final simulated arrangement; it does not play a
falling animation in the viewport.

## Check in the editor

1. Create a real floor brush and a suspended **512 x 512 x 16** slab above it.
   Select only the slab and open **Plugins > Crumble**. Leave Through axis on
   Auto, with 3 holes, 6 sides, diameter 24..48, spacing 8, 4 fragments per hole.
2. Apply. Check three through-holes, 12 visible selected fragments resting on
   the floor and the **Crumble fragments** group in the Map tree. The remaining
   slab must stay outside that group. Select/move the group as one object.
3. Undo once: falling and the new group disappear while the cut/fractured plugs
   return. Undo again: the original slab returns. Redo both steps.
4. Repeat on a vertical wall. Automatic release should use the camera-facing
   side. Test manual axis/side and **Frame fragments**.
5. On the thin slab explicitly select Y with diameter 24. The preflight should
   explain the incompatible section and leave the map unchanged. Also try an
   excessive diameter, spacing or count.
6. Try different seeds and a tighter valid layout. Confirm the reported actual
   seed, bounded attempts and absence of leftover brushes from failed attempts.
7. Move/change selection during a pending operation. The stale result must be
   rejected. Cancel physics and check that generated geometry can be undone.
8. Remove the supporting floor or use an extremely short Duration. The plugin
   must not report resting success for fragments still flying away; restore a
   floor or increase Duration and retry.
9. Save/reopen a copy of the result. Check fragment entities, grouping and
   materials. Repeat the slab/wall checks on Windows and Linux SDL2/native.

Automated checks cover the independently loaded plugin, real geometry/Bullet
pipeline, render-tree membership, grouping, retained selection, Undo/Redo,
guarded input, certified-layout fallback and cancellation/physics diagnostics.
The standalone host-double test also covers exhausted bounded searches and
no-drop messaging. Game files and original user maps are not rewritten.

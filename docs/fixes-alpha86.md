# Alpha 86: large selection, Duplicate and document WAD order

## User failures reproduced

The Windows qconsole.log reported two VM runaway failures on E1M1:
`fb_clone_groups -> fb_tx_set_key -> fb_tx_entity_set -> editor_ents_reload`,
and `fb_selection_set -> fb_entity_select`. The first reparsed all entities for
individual property writes; the second repeatedly searched and copied a growing
selection list while selecting a closed group containing 22,880 point entities.
These failures shut down CSQC and left an empty/internal-server-error viewport.

## Changes

- Index selected entity IDs and grow their storage geometrically. Batch removals
  compact the list once while preserving its order.
- Join selection history strings once, rather than creating quadratic temporary
  strings. Restore the complete snapshot without command-token count limits.
- Refresh only the changed entity for property writes, Undo, Redo and remote
  updates. Grow entity storage geometrically and reuse empty engine slots.
- Keep box vertices in independent buffers, so moving the entity array does not
  invalidate cached vertex pointers. Release buffers on update and full reload.
- Index clone destinations and point IDs; precompute child-geometry membership
  and new group IDs. Preserve independent cloned groups and parent links.
- Parse ordinary unescaped entity fields directly; keep the existing escape
  normalization for legacy paths and quoted/backslash values.
- Keep threaded FTE temporary-string marking on an immutable snapshot. Sweep
  strings and modify the live table on the VM thread, checking snapshot identity.
  The stress test exposed an additional loader-worker crash in PR_QCGC_Thread;
  the host allocator and live table are no longer mutated by that worker.
- Keep the existing 512 MiB VM arena and runaway protection. No visual settings
  or retained material rendering settings are reduced.

## WAD resolution

A brush face stores the material name `clip`, not a binding to a particular WAD.
The original E1M1 FTE.map orders tools.wad after the other matching archives.
The E1M1 FTE (2).map used for performance tests has a different WAD list: the last
matching archive there is MG3Tex.wad. Multiple archives contain different `clip`
images. This is a resource-order difference, not proof of different geometry.

Keep a separate document WAD list instead of appending it to global profile
settings. Its ordered archives override general libraries, matching the reverse
mount lookup used by TrenchBroom. Promote explicitly referenced archives even
when they were also discovered automatically. Resolve relocated WADs by a unique
available filename; ambiguous matches remain unresolved instead of guessing.
The compiler retains the equivalent reversed first-match search list.

An already rewritten MAP header cannot reveal the author's previous WAD order.
Opening the original MAP restores its own order; this update does not rewrite
original maps or WAD files automatically.

## Validation

- Actual E1M1: select and deselect the group of **22,880 point entities**, Duplicate,
  Undo and Redo; verify all 22,880 selected IDs exist after each operation.
- The full copy completed in approximately **4.6 seconds** on the test machine.
  It remains a large synchronous edit, not an instantaneous operation.
- A 42-brush group also copied and survived Undo/Redo in the large map.
- Owner slots stayed bounded: 49,392 engine slots after the 22,880-entity copy,
  with cache capacity 52,576, rather than allocating a new block per entity.
- Production GC harness: worker marking performs no host frees; changed slots
  and strings created after the snapshot survive; VM-thread sweep frees only the
  intended dead string.
- Production WAD resolver tests cover ordered promotion, relocation, case-insensitive
  filename matching and ambiguity. An audit with the actual available WADs and
  original E1M1 header resolves `clip` to **tools.wad** (21 references resolved;
  ubiLight_v103.wad remains unavailable in this fixture).
- Windows and Linux native/SDL2 use the same QC, native source and engine patch.
  Virtual Arch/KWin checks exercise lifecycle and fallback rendering, not a
  physical Linux GPU/input test.

The retained rendering optimization from alpha 85 is included. Its published FPS
measurements are historical; alpha 86 is a correctness release and does not claim
new FPS measurements.

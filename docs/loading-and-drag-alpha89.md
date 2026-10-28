# Alpha 89: large entity drag crash and loading investigation

## Fixed: the viewport disappears while moving a large entity group

The E1M1 reproduction selects 22,880 misc_drip entities. Alpha 88 can abort CSQC
as soon as its move preview is drawn. The stack reports fb_transform_replaces
called from fb_visual_prepare, followed by `runaway loop error`, CSQC shutdown
and the failed internal-server reconnect. This is not loss of the MAP file.

Each selected entity previously searched the entire transform snapshot to decide
whether its original should be hidden. Rendering and preview bounds repeated
these searches, making the gesture quadratic in selection size.

- Captures now index point IDs directly and large brush selections by model/ID.
  Clearing or recapturing a gesture invalidates the index. Duplicate previews
  retain their original objects, as before.
- Large point previews submit the same translated wire boxes through one native
  call, with the same shader, depth behavior, edge order and color. Model-backed
  entities still draw their transformed instances. The existing fallback is kept.
- Original-overlay candidates are filtered once per capture; moving points are
  rejected before preparing their original bounds. Plain translation reuses the
  union captured at gesture start. Affine/extrude previews keep their own bounds
  computation.
- Transactions with more than 4096 records can retain brush geometry when they
  only update existing point/group records. The whole batch is validated first.
  Large creates/deletes, changed owner class/model/type and unsupported geometry
  retain the full loader fallback. This applies to Undo and Redo as well.

## Loading: comparison with TrenchBroom

[TrenchBroom MapReader](https://github.com/TrenchBroom/TrenchBroom/blob/master/lib/TbMdlLib/src/MapReader.cpp)
parses an input view and then creates independent entity, brush and patch nodes
through task-manager jobs before assembling their relationships. ReyondBroom
also has FTE model loading, the QC document tree, material resolution and the
packed grid/surface caches. Parallelizing the VM itself would be unsafe.

The native open path now reads one immutable MAP snapshot for profile detection,
WAD discovery and staging, instead of reading the full source three times.
Repeated WAD indexing can reuse an image only after it has previously passed
byte validation or been written successfully. It checks immutable decoded-source
identity, palette and image size/modification time; changed sources fall back to
byte comparison and collision-safe filenames. Weak source references preserve
the decoded WAD cache's memory bound. Winning texture pixels remain owned
independently so a few surviving textures cannot pin many complete source WADs.

Profiling uses the existing `ftebroom_profile_commit` switch and adds no new cvar.
The log distinguishes source reading, resource/staging preparation, Q1 material
indexing and the existing QC geometry stages.

## Results and limitations

Windows, RTX 4060 Laptop / Core Ultra 7 155H, owned E1M1 copy with actual WADs,
30,327 brushes / 189,772 faces and all 22,880 misc_drip selected. Original MAPs,
WADs and mod files were not modified.

- Preview, Cancel, Move, all-coordinate checks, Undo and Redo pass. All 22,880
  selected IDs remain valid, and brush geometry epochs stay unchanged on Move,
  Undo and Redo. A direct UI query still returns every misc_drip property record.
- The final 1080p preview interval averaged **52.03 FPS**, median 18.2 ms, p95
  23.2 ms, p99 26.7 ms. That eight-second diagnostic interval also included one
  full membership/origin check; its 209 ms maximum is not a clean steady-frame
  latency measurement. Alpha 88 aborts on the same preview, so there is no valid
  before-preview FPS comparison. This is not a 60 FPS floor claim.
- Move plus document refresh measured **2.74 seconds** for the full group.
  The operation is still not instantaneous, but no longer rebuilds all brushes.
- Loading samples: reading the 32 MB MAP took about 20-34 ms; native resource/
  staging work 104-135 ms in comparable runs. Repeated material indexing measured
  **1.35 s before / 0.89 s after**. Cold indexing still takes several seconds;
  the cache is not a promise of faster cold startup.
- The fully prepared large-map marker still arrives roughly ten seconds after
  the native open request. QC document rebuilding, surface/grid preparation and
  engine resource work remain substantial. This release does not implement a
  full parallel loader equivalent to TrenchBroom.

The next substantial loading change should move independent face/UV/cache
preparation into native jobs over immutable input, resolve materials once per
material, and publish completed buffers on the engine thread. A persistent cache
must validate MAP/resource/profile versions and reconstruct identities rather
than retaining VM pointers. That architectural change is separate from this
crash fix and the bounded texture-cache improvement.

## Validation

- Windows: full 22,880-point preview/Move/Undo/Redo; three-brush transforms,
  duplicate/cancel, guide-ray reference and linked-group tests; mixed patch/entity
  transforms with the Q3 patch feature enabled; renderer restart.
- Linux native and SDL2: live 600-point previews, unchanged source-coordinate
  checks, the native preview capability and renderer restart under virtual Arch
  / Plasma 6.8.1 / KWin Wayland. These are software-rendered correctness checks,
  not physical Linux performance certification.
- On the native Linux fixture, native and reference preview screenshots match
  exactly: zero changed pixels across 1280 x 800. Material alias scripts also
  remain byte-identical on E1M1.
- WAD/image-cache tests on Windows and Linux cover canonical paths, immutable
  source reuse, changed WADs, palette/source/index changes, image metadata changes
  and removed image files. Engine patch reconstruction and package/source hashes
  are checked before publication.

## Please check

1. Select the large misc_drip group, drag horizontally and with Alt vertically,
   release, then Undo/Redo. Ensure all entities move once and the viewport stays
   alive. Repeat several gestures; try Cancel with Esc and Duplicate + Move.
2. Check individual model-backed entities, small brush groups and mixed selections;
   originals should disappear during Move and remain during Duplicate previews.
3. Check selection bounds, linked-group synchronization, grid/outlines and a
   renderer restart after moving entities.
4. Reopen E1M1 with the same WAD list. Check CLIP, TechTrim and masked textures;
   reconnect or edit a WAD and verify its resources refresh. Compare first load
   and reopening separately when reporting timings.
5. On Linux native and SDL2, repeat drag/Cancel/Undo/Redo and resource reloads.
   If the viewport still aborts, send the new crash file and last console lines.

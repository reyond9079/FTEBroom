# Material loading and face selection — alpha 105

## Quake II loose WAL files

Quake II normally stores WAL files below `textures/`. The editor also indexes loose WAL files at the selected mod root. An exact namespaced material wins; when it is absent, a root WAL with the same basename can resolve a MAP reference such as `dw/floor_wide` to `floor_wide.wal`. Empty or stale internal WAL names do not replace the file identity. Original files are not rewritten.

The same source pixels and dimensions are used in the browser, MAP renderer, compiler staging and preview aliases. This does not recover resources that are genuinely absent.

## Raw MAP preview

The preview process needs a private copy of generated WAD/WAL and approved placeholder images. The old validator accepted only the Quake I cache prefix, producing `Invalid raw-preview WAD image path` for other generated cache families. It now accepts all supported generated image directories while rejecting traversal and absolute paths. MAP geometry and source game resources remain unchanged.

## Selecting faces

With whole brushes selected, Shift targets only those brushes. After selecting an individual face, Shift-clicking a different brush switches to that face. Ctrl+Shift-click adds another brush face. Hover outlines follow the same scope.

## Service material opacity

Open **Edit > Preferences > View > Service materials**, or **Map > View options > Renderer**. Adjust **Service texture opacity** from 0 (invisible fill) to 1 (opaque). The default is 0.35. The setting persists and applies to recognized clip, trigger, caulk/nodraw/skip, hint and portal surfaces in the editor; ordinary material transparency and game rendering are unchanged. Changes apply when the slider is released.

## Verification

- Supplied root `floor_wide.wal`: real 64×128 pixels, logical MAP alias lookup, native BSP compilation without a placeholder, and byte-identical compiler input.
- Face-selection regression checks: whole-brush scope, cross-brush replacement and additive selection.
- Opacity checks at 0, 0.12 and 1; ordinary surfaces unchanged.
- Missing-material GPU MAP bake and raw-preview staging: generated placeholder accepted without leaving the baked view.
- Generated-image path tests reject traversal, absolute paths and unsupported extensions.

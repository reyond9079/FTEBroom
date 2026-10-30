# Quake II material consistency — alpha 106

Quake II resource resolution now publishes an exact catalogue entry for every material used by the opened MAP that can resolve to a loose mod-root WAL. For example, `ls/ev_main` and root `ev_main.wal` share the same decoded image rather than depending on a fallback in only one drawing path.

The material browser and inspector use the same resolver. A stale generated placeholder no longer wins over an available real root WAL. Classic Quake II material paths normalize case and Windows separators consistently. UV editing, texture alignment, brush-to-patch conversion and OBJ UV generation use the resolved WAL dimensions instead of the dimensions of an already-registered missing-image shader. Other game profiles retain their existing image-size lookup.

Source MAP and WAL files are not rewritten. Exact namespaced resources retain priority over root-basename fallback. The editor does not invent original pixels for resources that are genuinely absent.

## Checks

The local `sl_1_02.map` fixture contains 23 drawable faces named `ls/ev_main`. The material regression compares the source face count with the cached render count and checks that every such face uses the same generated material. It also checks the exact catalogue alias, real 64×48 image dimensions, selected-face dimensions, Windows path normalization and replacement of a stale placeholder by a real WAL. A disposable six-face brush exercises normal, uncached and drag-preview rendering.

To test your map, restart the updated editor, reopen the map, inspect the previously affected faces, select/deselect them, move a textured brush and check UV Fit/Scale. Missing resources still require their original files or an explicitly approved placeholder.

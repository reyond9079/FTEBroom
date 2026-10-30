# Alpha 110: Quake II legacy liquid-contents compilation fix

The earlier compile-copy repair covered `SOLID | WINDOW` only. Older editor exports can also contain an inferred SOLID bit alongside explicit WATER, LAVA, SLIME, AUX or MIST contents. QBSP rejects those combinations as mixed visible contents.

The disposable compiler input now removes SOLID when exactly one other Quake II visible type is present. Higher contents bits, surface flags/values, material names, UVs and geometry are preserved. Ambiguous combinations containing multiple non-solid visible types are not guessed or silently changed. Original source maps remain untouched.

The existing engine fix continues to preserve explicit Quake II contents when a source map is reopened. The expanded compiler-input repair also handles already affected exports and sessions.

## Verification

The reported `sl_1_02.map` build snapshot was available locally. Its 74 WINDOW faces had already been repaired, but 86 WATER faces still contained 33 (`SOLID | WATER`) instead of 32 (`WATER`). Only those 86 contents fields changed in the test copy. QBSP and LIGHT completed on the repaired copy. Unit tests cover all six non-solid visible types, classic/Valve 220 syntax, detail bits, surface flags, unchanged valid contents and ambiguous combinations.

A separate leak remains in this map: QBSP reaches `key_power_cube` at `(-1944, -744, -368)` and does not produce a normal portal file. The same leak is reported when compiling a direct copy of the original MAP without the editor export, confirming it is independent of this contents repair. VIS cannot complete until the leak is sealed. For a temporary BSP/light preview, disable VIS explicitly in Setup compile; the editor does not automatically change your geometry or silently disable stages.

Some texture warnings remain because the original mod resources are unavailable. Placeholder messages are not the cause of the mixed-contents error.

## Please check

1. Reopen the map and compile. Already affected sessions/exports should log repair of the conflicting visible-contents fields.
2. Confirm `Mixed visible contents: SOLID | WATER` no longer appears.
3. For this particular map, seal the documented leak before enabling VIS, or explicitly disable VIS for a temporary preview.
4. Confirm water/glass contents and transparency, plus normal saving and Undo/Redo. Source MAPs and their textures are not replaced by this repair.

# Alpha 41 — single-folder Quake 1 and Wayland cursor

Quake 1 now has one **Mod directory**. Maps, models, palette and WADs are read from that directory and its PAK archives. The previous separate base/source directory is no longer mounted. Migration prefers the old mod setting, or the old base folder when the mod was empty. LibreQuake remains available. A mod that depends on files from another game directory must contain those required resources to work in this single-folder configuration.

In the Linux editor, cursor shapes use ImGui's final drawing layer. The compositor cursor is hidden inside the editor; gameplay and Windows keep their existing cursor path. This avoids hardware cursor buffer changes when crossing dock splitters. The shape and hotspot still follow the selected ImGui cursor.

Build continues to compile an isolated `snapshot.map` exported from the live document, including unsaved edits. This is intentional and protects the source MAP. Logs now identify the original map, export path and revision, and refuse an export if the active document changed. A snapshot filename alone is not evidence of compiling another document.

The entity definition loader now logs whether `info_player_start`'s model exists in the selected mod. The reported missing player model and compilation failure still require the affected mod/file information and complete compiler error; they are not claimed fixed by this release.

## Checks

- Open Preferences: only one Quake 1 mod directory should appear; restart and check persistence.
- Confirm old settings select the former mod, not the old source folder.
- Cross and drag dock splitters, enter text fields, use camera look, and Alt-Tab on Wayland. Only one cursor should appear, with the correct resize shape.
- Add `info_player_start`. Check the console line identifying its model and `found` / `MISSING`; compare scene and entity browser.
- Build Q1 and Q3 maps, including an unsaved geometry change. Check the source path in the log, the isolated build copy and the resulting BSP.
- If compilation fails, provide the error lines after the compiler starts, not just the `snapshot.map` path.

Tests run in isolated Arch userspace with Weston under WSL2/software OpenGL; physical Arch desktop testing is still required.

The ImGui cursor layer described above was later replaced by native shape cursors: Wayland uses `wp_cursor_shape_v1`, X11 uses Xcursor theme shapes, and the ImGui image is retained only as a fallback.

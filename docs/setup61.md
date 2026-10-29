# Game and mod setup

On Windows, start **ReyondBroom.exe** directly. It is a GUI executable: it starts the
bundled engine without CMD, sets the installation directory, and passes the editor
startup options. Shortcuts can use any working directory; Unicode/spaces are
preserved. Keep the complete extracted distribution together.
The legacy CMD entry point forwards to this executable.

The first-time wizard keeps the same window size for Language, Interface size, Theme, Game and
Mod/resources. Choose Quake 1, Quake II, Quake III Arena, Half-Life or FTE Custom game using the large
game rows. Quake III and custom FTE games retain their existing resource settings.

For **Quake II**, use **Browse...** to select the installation root, `baseq2`, or
a mod directory. An installation root resolves to `baseq2`; a mod also reads a
sibling `baseq2`, with mod resources taking precedence. WAL textures and PAK
archives are loaded. Bundled ericw-tools compile IBSP 38; no compiler path is
required during setup. Optional entity definitions are under **Advanced settings**.

For **Half-Life**, select the installation root or `valve` and choose **FreeHL**.
For Counter-Strike 1.5, select `cstrike` (or the installation root) and choose
**FreeCS**. A sibling `valve` supplies base resources. The folder is required;
WAD3 archives and entity definitions are optional overrides under **Advanced
settings**. WAD palettes are read from each texture. Bundled SDHLT compiles
GoldSrc BSP 30; enable LIGHT / RAD in **Setup compile** for lightmaps. The runtime
is installed as a separately named editor package; original game files are not
replaced. FreeHL/FreeCS requires retail resources and does not execute arbitrary
GoldSrc mod DLLs.

The selected game/mod directory is normalized and saved, including paths outside
the editor installation and paths containing spaces. Invalid paths keep the
wizard open and show an error naming the selected game.

**Play mod** starts player control by default. Half-Life / Counter-Strike uses
first-person view with viewmodels enabled; the embedded Windows preview still
offers **Free camera** for inspection. Click the viewport to capture controls,
and press Escape for the preview controls menu. The editor uses its own scaled
cursor images instead of inheriting a game cursor. Set **Cursor size** under
**View → Preferences → View** or **Workspace → Appearance…**. The shared 50–250%
setting applies immediately to pointer, text and resize shapes, independently of
UI/font scale, and survives restarts. **Reset cursor size** restores 100%.

If the current editor contents have not been compiled, **Play mod** asks whether
to **Compile changes and play**, **Play last compiled BSP**, or **Play current MAP
without compiling**. Compiling rebuilds geometry and launches only after success;
errors/cancellation retain the previous BSP. The last successful build is also
remembered across editor restarts. A restored build is treated as unverified, so
it never silently replaces newer editor contents.

MAP preview exports an isolated snapshot from memory, including unsaved changes,
and loads it directly with FTE. It does not save over the working MAP or replace
the last compiled BSP. Used WAD images are staged privately for classic profiles.
This mode has no baked lightmaps or compiled visibility and can load more slowly
than BSP. Unsaved untitled maps can use MAP preview; compilation requires choosing
a save location first. The last-BSP option is disabled when no build is available.

For Quake 1, select **LibreQuake** to download the pinned v0.09-beta mod, or
**Choose my own mod** and **Browse...** to select a directory. Downloads run outside
the UI thread, report progress, validate SHA-256, and extract only inside the
editor's download directory. Linux needs Python 3. The downloaded mod is selected
after successful installation. Monolith is not included.

**Advanced settings** is collapsed initially. It contains additional WAD2 archives,
an optional entity definition override, the MAP format and the target BSP format.
If the definition override is empty, the editor searches the mod for DEF, FGD and
ENT files, including PAK/PK3 members. Loose definitions take precedence over packed
definitions; top-level files take precedence over nested examples. If several
candidates remain, filename order is used; use the override to choose another.
FGD includes keep their source directory, also inside archives.

The pinned LibreQuake archive contains no DEF/FGD/ENT files, so its bundled
LibreQuake definition remains the fallback. Other mods fall back to Quake 1
definitions only when the mod supplies none.

Close **Game / resources** with its X or **Close**. Closing does not require valid
resource paths and does not apply changes. The dialog opens again only on request
or a new resource-loading failure.

## Icon attribution

Quake and Quake III identification icons are from TrenchBroom's game configurations
(`app/resources/games/Quake/Icon.png` and `Quake3/Icon.png`). Quake trademarks belong
to their respective owners. The FTE icon is from the FTE source tree
(`engine/droid/res/drawable/icon.png`). These identify the selected game/engine;
they do not imply endorsement.

## Checks

- Click X on the first-time wizard; it must close without opening another dialog.
- Open Game / resources, close it with X and Close, and reopen it from the menu.
- Choose each game row and check that the next page matches the game.
- Download LibreQuake; check progress, extraction, selection, textures and entities.
- Select a mod with Browse, use Up, and verify the chosen directory persists.
- Expand Advanced settings and override the automatically discovered definitions.
- Verify invalid paths and network errors remain recoverable without trapping the UI.

Interface size offers Small, Medium, Large, Extra Large and Largest in a large single-column list, with immediate application and persisted selection. The launcher checks required runtime DLLs before starting; the Windows packager validates transitive dependencies.

Selecting LibreQuake does not start a download immediately. Press **Load resources
and start**: the progress bar appears inside the LibreQuake row, then resources
load automatically. There is no separate download button or activation checkbox.
Closing the dialog during a download cancels automatic startup; the background
installer can finish for use next time. First-time setup never opens Recently
edited maps; later launches show the list when it contains existing history.

## Rotation tools

Choose **Tools > Rotate Grid**. **Pick center** lets you click a surface or the
viewport's working plane. You can also enter Center coordinates or drag the
central handle. Drag an X/Y/Z rotation ring as in Rotate. **Step (degrees)**
controls angular snapping (1 degree by default); **Apply** rotates by that step
around the chosen axis. **Reset grid** restores the original origin and axes.
Escape during a drag cancels that drag. Grid rotation changes snapping and grid
drawing without moving map objects. The grid orientation is session state.

The ordinary **Rotate** tool now also has **Pick center**. Its center changes
remain undoable; Reset center returns to the selection center.

## Materials and files

LibreQuake installation also downloads the pinned **dev.zip**, verifies its
SHA-256, and connects its 14 WAD archives automatically. The WADs are copied into
`lq1/texture-wads` without overwriting edited copies. Existing mod installations
only download the missing development package. Build stages these WADs with the
same precedence as the texture browser.

Drag a material tile onto a brush face or patch to apply it to that surface. Undo
reverts the operation. Dropping onto empty space only chooses the creation material.
Open a WAD in Mod file manager like a folder to see texture thumbnails; double-click
a texture to open the image viewer. The virtual textures can be copied as TGA
images. WAD contents are read-only in this browser; use the existing image importer
to add textures. **Setup build** opens compiler settings; **Build** starts the build.

# Leak route inspection (alpha 111)

## Automatic detection

After geometry compilation, the viewport Compile pipeline checks its own snapshot directory for a non-empty `.pts` or `.lin` point file. Detection also runs when a compiler exits with an error, so leak-test failures can still expose their route. Only diagnostics belonging to the current source document and map generation are loaded; arbitrary paths from compiler log text are not executed or followed.

A detected route appears in the editable 3D/2D viewports and opens **Leak route**. A successful leaky build stays in map-editing view instead of switching automatically to BSP. The BSP remains available through **Show BSP**. A successful geometry rebuild without a leak file clears the previous route. Switching maps unloads the old route.

## Inspecting the route

- **Focus leak** moves the active camera to the selected point and looks along the path.
- **First / Previous / Next / Last** navigate the point list. You can also enter a one-based point number.
- Existing **View > Next Point / Previous Point** commands and the period/comma shortcuts use the same selection.
- **Show route** hides or restores the overlay without unloading its data. Closing the panel leaves the overlay unchanged; **Leak route** on Tools reopens the panel. **File > Unload Point File** removes it.
- The red path is visible through geometry. Its two endpoints are green and blue; the selected point is yellow. Moving away from a focused point makes its marker visible again.
- The panel displays the selected coordinates, source point-file path and an **Outdated route** warning after map content changes. Recompile to replace stale diagnostics.

The route connects an entity with the outside. Neither endpoint necessarily identifies the opening. Follow the route through walls/ceilings to locate the gap. The editor does not seal holes, move entities or change map geometry.

## Files and performance

Manual point-file loading accepts both `.pts` and `.lin`, including optional parentheses around coordinates. Consecutive duplicate points are removed. Invalid/non-finite coordinates, empty files and excessive input sizes are rejected before the current overlay is replaced.

The route mesh is built once when the file loads and submitted as one line batch; the editor does not test every brush or rebuild the path every frame. Markers are drawn separately. No route overlay is submitted when hidden or unloaded.

## Verification and checklist

Automated Windows/Linux parser tests cover both extensions, duplicate points, valid coordinate forms and malformed input. Runtime tests include a real background compile of `sl_1_02.map`, automatic point-file discovery, source/revision binding, camera positioning, mesh caching, stale detection, first/last clamping, hiding/unloading and unchanged document history.

Please check:

1. Compile a leaking map. Confirm the path and Leak route panel appear even if VIS subsequently fails.
2. Use Focus, First/Last, Previous/Next and the point-number field; verify camera movement and orientation.
3. Inspect the route in perspective and orthographic views. Check endpoint/selected markers and visibility through geometry.
4. Hide/show the route, close/reopen its panel, and unload it through File.
5. Edit geometry: the panel must mark the route outdated. Switching maps must clear it.
6. Fix the leak and rebuild geometry: successful compilation without a leak file must remove the old route. Existing maps, WADs, autosaves and normal compilation remain unchanged.

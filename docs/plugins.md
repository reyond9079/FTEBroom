# Native editor plugins — API v1

Open **Plugins > Manage plugins...**. This panel shows the installation directory,
installed plugins, load errors and Enabled/Open controls. Copy each plugin's
folder there and choose Rescan plugins. Third-party plugins start disabled;
enable them explicitly. The bundled Crumble example is enabled on first use.
Enabled choices are retained. Running plugins cannot be unloaded/rescanned
until their operation finishes. Closing their settings window does not cancel
their job; the example provides Cancel physics while simulating.

These are native libraries running with the editor process's permissions, not
sandboxed scripts. Install trusted code. A mod, MAP, PAK or PK3 cannot register
a plugin through its virtual resource files. Windows needs a Windows x64 DLL;
Linux needs a Linux x64 SO. They are separate builds of the same source.

## Crumble example

Choose one whole convex brush, then **Plugins > Crumble: Perforate, Fracture,
Settle**. Configure hole count, polygon sides, through-axis, diameter range,
spacing, fragments per hole and seed. Auto through-axis is the default: it
prefers the thinnest feasible section, so horizontal slabs cut along Z.
Physics exposes friction, bounce,
initial impulse and duration. Choose Apply.

Release fragments outside the wall is enabled initially. It translates the
selected fragments just beyond the original brush along the through-axis,
keeping their relative positions, before physics starts. Automatic side releases
horizontal slabs downward and vertical walls toward the editor camera. Disable
Automatic release side to choose positive/negative manually, or adjust clearance.
Disable release to start inside the holes;
tightly fitted pieces may then jam against the wall instead of falling.

The editor cuts through-holes, retains their plugs as `func_wall`, fractures
only those plugs and runs Bullet in the background. Fragments collide with
the remaining wall, visible brushes and patches. Generated fragments stay
in their plug entities and remain selected after falling. The plug entities are
parented into one **Crumble fragments** group, with the remaining wall outside
it. Grouping shares the falling Undo step. A floor must exist
under them; the plugin does not generate an artificial ground plane.

Before trying seeds the plugin projects the convex planes onto the through-plane,
including a positive thickness margin, and checks minimum diameter, count and
spacing. This is a conservative **enclosing clearance circle** contract; a very
narrow section can be rejected even if a specially oriented polygon could fit.
Area and distance bounds prove some requests impossible under that contract.
A constructed separated circle layout proves feasibility when found. Other
cases remain inconclusive; failed random packing is never presented as a proof.
At most 16 geometry seeds are tried. The first eight use random diameters within
the range; remaining attempts use the certified minimum-size layout when available,
while the polygon shape and fracture still vary by seed. Failed attempts roll back.
Changing the map/selection stops the search instead of touching a different brush.

Physics retries at most eight seeds, retaining the valid geometry. Results still
moving faster than 2 map units/second or rotating faster than 0.05 radians/second
are not applied: add supporting ground or increase Duration. These are practical
rest thresholds, not a guarantee against all numerical collision errors. Completion
reports moved/dropped counts, mean drop and the geometry seed. A no-drop result is
explicitly identified. Frame fragments after applying is enabled initially; a
separate Frame fragments button is also available. This is an offline simulation
that applies the final arrangement, not a live falling animation in the viewport.

Limits: holes x sides <= 128, total fragments <= 128, and 0.1..20 seconds.
Geometry failures roll back the complete holes/fracture batch. If physics
fails or is cancelled, the generated geometry remains available for inspection.
Undo once reverses applied falling; Undo again restores the original brush.
Redo restores both steps. Changing the document during simulation invalidates
the pending physics result rather than applying it to unrelated geometry.

## Create a plugin

The SDK is [reyondbroom_plugin.h](../sdk/reyondbroom_plugin.h). It uses a versioned
C ABI, without engine or ImGui structures or shared C++ allocation. The full
example is in [plugins/rubble](../plugins/rubble). It is built as its own library;
the editor contains the loader and generic tool interfaces, not its UI logic.

1. Copy the example. Change its manifest ID, display name and binary filename.
2. Export `ReyondBroomPluginInit(const RBHostV1*)`. Check `api_version` and
   `struct_size`, then return static `RBPluginV1` metadata with a matching ID.
3. Implement `draw` for the plugin window, `tick` for asynchronous state, `busy`
   to protect running work from unload, and `shutdown` to release owned work.
4. Build for the editor's OS/architecture; copy the binary and `plugin.ini`
   into one folder under the installation's `plugins` directory.
5. Rescan, enable and open it from the Plugins menu.

Build the example with `python scripts/build-plugins.py --output out/plugins`,
optionally `--compiler /path/to/g++`. Alternatively use
`cmake -S plugins/rubble -B out/rubble` and `cmake --build out/rubble --config Release`.
MSVC, MinGW and Linux C++ compilers can use the header; distribute all required
runtime dependencies. The supplied Windows build uses static MinGW runtimes.

## Host functions

- `number`, `string`, `set_number`, `set_string`: editor variables. String reads
  copy into caller-owned buffers. Use a unique prefix for plugin-owned settings.
- `command`: queue editor commands on the main thread. Completion is asynchronous;
  inspect status variables in `tick`, rather than assuming immediate execution.
- `import_map`: stage a MAP fragment up to 16 MiB at its original coordinates
  through the editor's undoable importer. A return of 1 means queued, not parsed.
- `settle`: start/poll/apply/cancel the shared Bullet tool. Check its return status;
  another operation may own the worker. Do not launch multiple simulations.
- Text, separators, numeric inputs, checkboxes, combos, buttons, SameLine,
  disabled sections and progress bars build ImGui UI using the editor's theme
  and font. `disabled(1)` begins a disabled block; `disabled(0)` ends it.
- `log`: write diagnostic messages to the editor console/log.

All host calls belong on the editor thread, inside its callbacks. UI calls are
valid only inside `draw`. Do not retain host-owned strings or call callbacks
after unload. The SDK currently supplies form widgets and editor-command/MAP
interfaces; it is not a direct mesh-pointer or skeleton-editing API. Native
plugin code can implement its own geometry generation, importers or processing.

`ftebroom_plugin_batch` composes `perforate`, `fracture`, `release`, `inset`, `bevel` and
`subdivision` in one transaction, with the current selection passed between
steps. Fracture accepts multiple selected plugs, up to 128 resulting fragments.
Set tool settings first, reset `ftebroom_plugin_status` to 0 and queue the batch.
Status becomes 1 or -1; `ftebroom_plugin_error` explains rejection. The example
then invokes the asynchronous Settle interface and waits for Apply to complete.

`ftebroom_plugin_selection` exports one whole convex source into
`ftebroom_plugin_source` (face count, low XYZ, high XYZ, then normal XYZ/distance
for each plane). Reset `ftebroom_plugin_preflight` to 0 before queuing it; 1 is
ready and -1 means an invalid selection. A batch starting with `guard` must match
this captured map/selection; -2 means stale input. Cancelled guarded batches
refresh their own rollback state so bounded retries remain possible. The bundled
example alone supplies `ftebroom_plugin_hole_layout` as optional U/V/radius triples
for a certified fallback; ordinary Perforate ignores it. Settle publishes moved,
dropped and mean-drop diagnostics without changing SDK API v1.

Native plugin crashes can still crash the editor. API mismatch and loader errors
are reported before callbacks run, but this is not process isolation. Save work
before testing third-party development builds.

# ReyondBroom online: experimental plot relay

This branch is separate from main. Alpha.70 is the normal editor release, including
the requested Linux SDL2 build. It does not contain the online experiment.

Experimental binary releases have an `-online.N` suffix and are marked pre-release.
Start with [the two-person quick start](../START-ONLINE.md); it does not require
building the editor from source.

## Scope

- Up to eight persistent, individually assigned 2048 x 2048 plots, with a 2048-unit height range.
- Edit your own MAP. Other plots are read-only BSP render models, not editable brushes.
- Fly throughout the combined scene; other participants appear as LibreQuake player models.
- Compile locally, then publish the last saved MAP and compiled BSP. Others receive the latest accepted BSP.
- View your last published BSP and return to the editable MAP without discarding live changes.
- Quake 1 / LibreQuake resources only. The server and clients compare PAK/WAD resource hashes.
  Uploaded embedded texture pixels must match the server's LibreQuake WADs.
- BSP2 only. Online compilation uses `qbsp -bsp2 -nofill`, retains collision hulls,
  and never invokes VIS. Open maps are accepted. LIGHT remains optional.
  Ordinary build profiles and their saved options are retained when disconnecting.

This is plot publication, not per-brush live synchronization. Uncompiled changes
remain private. The server stores and validates files; it never starts a compiler,
loads a map into an engine, or executes uploaded game code.

**Compile builds only your own plot.** Remote plots exist exclusively as render
entities and never enter the MAP export or compiler input. Publishing addresses
only the plot assigned to the session token. Recompiling your plot does not rebuild,
replace or modify another participant's saved MAP or BSP.

## Local setup

Use two separate writable editor installations for a two-person/local test. Do not
run both clients from one portable directory: their document and session files differ.

1. Build this branch using the normal build scripts. Python 3 is needed by the
   experimental client on Windows as well as Linux. On Windows, enter its executable
   path in the online panel if `python` is unavailable.
2. In each editor, load the Quake 1 profile and select LibreQuake. Both installations
   need the same LibreQuake v0.09 PAKs and `texture-wads` collection.
3. Start the relay from the source checkout:

```sh
python3 runtime/online/server.py --data /path/to/private-online-state --librequake /path/to/mod_lite/lq1
```

For an installed experimental build, replace `runtime/online/server.py` with
`ftebroom/online/server.py`. Windows can use `python` instead of `python3`.

4. Open **Workspace > Online session (experimental)**. Enter `http://127.0.0.1:28777`,
   a display name and your Python executable. Save your current work, then join.
5. Joining opens your assigned MAP. A cyan box outlines your editable plot.
6. Edit inside it, save, and press Compile. Then choose **Publish last saved MAP +
   compiled BSP** in the online panel. Use its BSP/MAP buttons to switch views.

The default relay listens only on localhost. Non-local hosting needs a separately
configured trusted network/TLS gateway. This prototype has bearer session tokens,
not an account/administration system, and has not been validated for public hosting.
Tokens stay in each client's editor-owned `data/online/session.json`. Keep that file
private. Assigned plots and latest sources/BSPs persist in the relay data directory.

Automatic stable-editor updates are disabled on this experimental branch. Use the
normal main-branch release in a separate installation for regular mapping.

## Admission limits

- 8 plots; 16 MiB per BSP; 4 MiB per MAP source.
- 16,384 rendered faces, 32,768 vertices/nodes/leaves, 128 submodels per plot.
- 8 MiB light data, 2 MiB visibility data; BSP-tree depth at most 256.
- Geometry must stay inside its assigned plot; nonfinite coordinates and invalid
  offsets, indices, cyclic trees or malformed visibility rows are rejected.
- At most one upload is validated at a time, 16 concurrent connections, a two-second
  upload interval and 64 published revisions per plot in this prototype.
- Client model-cache refreshes are bounded; restart when the experimental cache limit is reached.

These are conservative starting limits, not a guarantee of identical client FPS.
The relay's work does not scale with real-time rendering complexity; clients still
draw all visible accepted plot models. Uploaded entity text is not spawned as game logic.

## Automated local checks

```sh
python3 tests/test-online.py --qbsp /path/to/qbsp --resources /path/to/mod_lite/lq1 --output /new/test-directory
xvfb-run -a -s '-screen 0 1600x1000x24' python3 tests/test-online-editor.py --deps /path/to/build-deps --resources /path/to/mod_lite/lq1 --output /another/new/test-directory
```

The first test builds two open MAPs with collision hulls, publishes them through a
local relay, checks private source retrieval, shared BSP downloads and unrestricted
camera movement, then rejects invalid, oversized and cross-plot geometry. The second
uses two actual SDL editor instances to exercise connection, edit rollback, local
compilation, publication, avatars, remote BSP models and the MAP/BSP view switch.
It then edits/recompiles one client while the other plot is visible and verifies
the changed own BSP and unchanged checksums of the other client's MAP and BSP.

## Manual checks

1. Join from two installations; verify different plots and visible moving avatars.
2. Make an edit outside your box: it should roll back. Check Undo/Redo inside it.
3. Build an open platform: BSP2 and collision hulls should be present; no VIS stage.
4. Publish it and verify the other client receives the model without changing its own MAP.
5. Switch to your published BSP, then back. Check editable geometry and selection survive.
6. Reconnect using the same installation; verify your plot and last saved MAP remain.
7. Try an oversized/cross-plot BSP and a foreign texture: the relay should reject it.
8. Disconnect, then check normal compilation options and profile settings are restored.

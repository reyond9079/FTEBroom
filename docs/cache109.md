# Alpha 109: scheduled maintenance and recent-project texture caches

Full texture-cache and preview-session cleanup now runs **only when the editor closes, at most once per 24 hours**. Startup performs a small staging-only sweep of reserved `_ftebroom_open_...map` input copies. Opening a map does not trigger cleanup. Repeated launches do not enumerate the texture-cache folders or parse the shader/history manifests for maintenance.

The texture caches of the **five most recently used projects** are pinned in addition to the currently indexed shader images. A project is identified by its source-map folder, game profile, resource/mod directories and (for WAD profiles) the resolved WAD set. Maps sharing the same folder and resource setup share one project entry. Returning to a project makes it recent again.

Resource indexing collects generated image names in memory. On a project change, the editor writes a small immutable metadata snapshot; it does not rescan the texture directories. Multiple editor instances cannot overwrite each other's snapshots. The scheduled full sweep retains the latest snapshot of each of the five most recent projects and removes redundant older metadata snapshots.

Pinned images are not evicted for age or budget. Other generated images retain the previous expiry/budget rules. An evicted older project may require cache regeneration when revisited. Resource indexing may still read WADs normally; this change prevents cleanup-induced regeneration for retained projects, rather than eliminating all resource-loading work.

Original MAP/BSP files, WADs and autosaves remain protected. Shared instance locks and filesystem-link checks remain in place. Invalid project history disables texture eviction for that pass. A full check can extend a scheduled shutdown; it does not run in the interactive editor or on map opening.

## Verification and checks

- Automated Windows/Linux tests cover 24-hour boundaries, clock rollback, staging-only startup, five-project retention under age/budget pressure, reopening an older project, corrupt-history safety and existing source/link protections.
- Open projects A and B, close/restart the editor, then return to A. Check that its generated texture images remain available.
- Restart repeatedly within one day. `ftebroom/cache-cleanup.log` should show `Staging only` entries rather than repeated `Full scan` entries.
- Check that source maps, WADs, autosaves and normal compile/preview workflows remain intact.

# Alpha 79: Quake 1 material identity and reloads

- Match WAD material names case-insensitively in renderer preview lookups and the selected-material thumbnail. Preserve the spelling stored in the MAP. This fixes available mixed-case materials, including TechTrim16_b from MiniPack.wad, becoming white in custom brush rendering.
- Give generated Q1 surface shaders a resource-generation identity. FTE reuses an already registered shader name without replacing its generator script; resource reloads must therefore not retain an earlier missing image or previous WAD image.
- Display TrenchBroom's __TB_empty unassigned-material marker with the editor's neutral texture. This is an editor alias; source maps and WAD files are unchanged.
- Identify an unavailable selected material in the inspector instead of silently implying that its pixels loaded.

## Resource audit

The supplied E1M1 FTE (2).map references 377 distinct material names. The 35 loose WADs inspected contain 6,028 distinct names. Of the MAP names, 334 have source data, 42 have no matching source texture, and one is __TB_empty. Twenty-five available material names use spelling that did not match the lowercase preview index; these occur in 55,980 face references. This count does not mean every such face was visible or rendered through the affected pass.

Absent texture pixels cannot be recovered from the MAP. The original material names remain available for reconnecting the correct WAD. This release does not replace absent artwork with invented textures.

## What to check

1. Open the supplied map with its WADs and inspect TechTrim16_b on ordinary and grouped brushes, selected and unselected. Check its thumbnail and UV scale.
2. Connect a previously missing WAD or replace a test WAD, reload resources and verify the surface uses the new image.
3. Confirm genuinely absent textures show an inspector warning; reconnect their containing WAD.
4. Check __TB_empty surfaces and verify Quake III/FTE Custom material behavior remains unchanged.

The editing/save/material performance investigation and proposed phases are in [performance-plan.md](performance-plan.md). Those optimization phases are not implemented by this material hotfix.

## Validation

The actual WAD-backed TechTrim16_b brush renders white with the previous case-sensitive lookup. With the fix, its viewport is pixel-identical to the lowercase-name reference. Windows and Linux checks exercise the actual material index, resource-generation shader identity and preservation of Quake III name handling. The full supplied MAP opens with the original external WADs; its SHA-256 remains `44c5dce383187921da2b47e9b8ce780334008f0597b9577c293846161625ae0e`.

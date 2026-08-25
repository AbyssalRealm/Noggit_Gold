# Noggit_Gold Public Testing Guide

Thank you for testing Noggit_Gold. The most useful reports are reproducible and narrow.

## Before You Start

1. Back up the map/ADT files you are testing.
2. Record the Noggit_Gold commit/build you are using.
3. Start with disposable test terrain before using valuable production work.
4. Do not use valuable lighting DBCs for first-time Light Editor write tests.

## Priority Test 1 — Auto Texture PASS1B UI

1. Open Texture Painter -> Auto.
2. Select one loaded ADT.
3. Confirm Selected Terrain Height shows minimum, maximum, and span.
4. Enter obviously mismatched Low/High ranges and confirm the warning is understandable.
5. Click Fit Heights and confirm the generated ranges now intersect the selected terrain.
6. Confirm the panel remains readable at a narrow dock width.

### Texture-slot browser test

Test Base, Low Ground, High Ground, and Cliff separately:

1. Click the slot texture preview or texture name.
2. Confirm the existing Noggit Texture Browser opens/raises.
3. Choose a texture that is not already painted on the ADT if possible.
4. Confirm only the intended Auto Texture slot changes.
5. Repeat for all four slots.
6. Confirm `Use Current` still assigns the currently selected Noggit texture to that slot.

## Priority Test 2 — Auto Texture Single ADT Regression

1. Select one loaded ADT.
2. Choose four clearly different textures.
3. Fit or manually configure sensible height ranges.
4. Apply Auto Texture.
5. Confirm:
   - low terrain uses the Low role where expected;
   - high terrain uses the High role where expected;
   - steep terrain uses the Cliff role;
   - transitions are not rigid chunk-shaped bands.
6. Press Ctrl+Z and confirm the prior texture state returns.
7. Press Ctrl+Shift+Z and confirm the generated Auto Texture state returns.

## Priority Test 3 — Adjacent-ADT Seam Proof

This is one of the most important public tests.

1. Load two genuinely adjacent ADTs.
2. Select both in Auto Texture.
3. Use the same four texture roles and ranges across both.
4. Apply once to both selected ADTs.
5. Inspect the shared ADT border closely.
6. Look for:
   - sudden texture discontinuity;
   - repeated/noise reset patterns;
   - mismatched blend weights;
   - cliff behavior changing abruptly at the border.
7. Undo and redo the entire two-ADT operation.
8. Save/reload the ADTs and inspect again.
9. If possible, inspect the result in the WoW client too.

## Priority Test 4 — 3 to 8 Selected ADTs

After two-ADT testing succeeds:

1. Try 3, 4, and eventually up to 8 loaded selected ADTs.
2. Confirm Apply affects only the selected set.
3. Confirm an unloaded selected ADT causes a safe refusal rather than a partial operation.
4. Confirm undo/redo treats Apply as one expected transaction.

## Existing Gold Features Worth Regression Testing

Reports are also useful for:

- Eraser / Clear Tool category-specific clearing;
- Eraser undo/redo;
- Chunk Mover copy/paste;
- 90/180/270-degree chunk rotation;
- Saved Chunks persistence and object carrying;
- Water liquid-surface cursor behavior;
- Light Editor Port to Light;
- Draw Current Only / wireframe selected-light identification.

## Light Editor Warning

The write-safety repairs are not yet considered fully live-proven across a DBC write/restart lifecycle.

If deliberately testing this path:

1. use disposable/backed-up copies of Light.dbc, LightParams.dbc, LightSkybox.dbc, LightIntBand.dbc, and LightFloatBand.dbc;
2. make one controlled change;
3. verify the changed records;
4. fully restart/reload Noggit_Gold;
5. verify persistence;
6. report the exact DBC build/layout used.

Do not risk valuable production DBCs for first proof.

## Good Bug Report Template

**Build/commit:**

**OS:**

**GPU/driver:**

**Map + ADT coordinates:**

**Tool:**

**Expected:**

**Actual:**

**Exact reproduction steps:**
1.
2.
3.

**Undo/redo result:**

**After save/reload:**

**Checked in WoW client:** Yes / No

**Logs/screenshots/video:**

## Test Philosophy

A report saying “it broke” is difficult to use. A report saying “on Azeroth_24_53, selecting Base from the Texture Browser changes Cliff instead, reproducible after restart” is extremely useful.

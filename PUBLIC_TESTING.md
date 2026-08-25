# Noggit_Gold Public Testing Guide

Thank you for testing Noggit_Gold. The most useful reports are reproducible and narrow.

## Before You Start

1. Back up the map/ADT files you are testing.
2. Record the Noggit_Gold commit/build you are using.
3. Start with disposable test terrain before using valuable production work.
4. Do not use valuable lighting DBCs for first-time Light Editor write tests.

## Live Auto Texture PASS1 Status

**SEMI-LIVE-PROVEN — awaiting more hands to break things.**

The initial author test passed. The current public source is ready for broader hands-on testing before Live Auto is promoted to the fully live-proven baseline.

Source introduction commit:

`79944bbb86e7fa191a29f8d7251c2cf292355c29` — **Add Live Auto Texture terrain reapply PASS1**

Live Auto is opt-in through:

**Texture Painter -> Auto -> Reapply after terrain edits**

The first pass is deliberately narrow. Eligible paths are Raise / Lower terrain sculpting, Flatten, Blur, and normal image-mask terrain sculpting through Raise / Lower. Selected-vertex mode, Script terrain mode, bulk/import terrain operations, Eraser/Clear, Chunk Mover/Saved Chunks, standalone Brush Stack/Stamp Mode, and ordinary texture painting are not intended to trigger Live Auto in this pass.

## Priority Test 1 — Live Auto Basic Raise / Lower

1. Open Texture Painter -> Auto.
2. Select one loaded ADT.
3. Assign four distinct textures: Base, Low Ground, High Ground, Cliff.
4. Configure sensible Low / High / Cliff ranges or use Fit Heights.
5. Apply Auto Texture once manually.
6. Confirm `Reapply after terrain edits` is initially OFF.
7. Sculpt with Live Auto OFF and confirm Auto Texture does not automatically reapply.
8. Enable `Reapply after terrain edits`.
9. Raise terrain through the High Ground range.
10. Release/finish the terrain stroke.
11. Confirm the local texture distribution updates after the terrain action finishes.
12. Lower terrain through the Low Ground range and repeat.

Report:

- whether the update happened once after the stroke rather than continuously every frame;
- approximate number of chunks updated if shown in status;
- whether terrain response remained smooth;
- whether any unrelated ADT or texture area changed.

## Priority Test 2 — Live Auto One-Step Undo / Redo

This is a critical proof point.

1. Enable Live Auto on one selected loaded ADT.
2. Make one obvious Raise or Lower terrain stroke that visibly changes Auto Texture distribution.
3. Press Ctrl+Z once.
4. Confirm BOTH return together:
   - terrain geometry;
   - generated Auto Texture state.
5. Press Ctrl+Shift+Z once.
6. Confirm BOTH return together:
   - sculpted terrain geometry;
   - generated Live Auto texture state.
7. Change Auto Texture UI thresholds after the stroke, then redo an already-undone action and confirm redo restores the cached result rather than unexpectedly recomputing from the new UI settings.

## Priority Test 3 — Live Auto Cliff / Flatten / Blur

### Cliff creation

1. Start with terrain below the configured Cliff threshold.
2. Steepen it through Cliff Start / Cliff Full.
3. Finish the stroke.
4. Confirm the Cliff role follows the new slope.
5. Undo/redo once each.

### Flatten

1. Flatten a previously steep textured slope.
2. Finish the action.
3. Confirm Cliff texture retreats appropriately.
4. Undo/redo once each.

### Blur

1. Blur a sharp slope.
2. Finish the action.
3. Confirm the generated cliff transition updates with the softened terrain.
4. Undo/redo once each.

## Priority Test 4 — Internal Chunk Edge

1. Enable Live Auto on one selected ADT.
2. Sculpt directly across an internal chunk boundary.
3. Finish the stroke.
4. Inspect the boundary closely.
5. Look for:
   - abrupt texture cutoff;
   - a visible local reapply box;
   - noise restart;
   - cliff discontinuity;
   - neighboring chunks failing to refresh.
6. Undo/redo the action.

Live Auto intentionally expands the local work set by a loaded neighbor-chunk ring inside the selected ADT coverage. Please report any place where that appears insufficient or unexpectedly broad.

## Priority Test 5 — Adjacent-ADT Live Auto Seam Proof

This is one of the most valuable public tests.

1. Load two genuinely adjacent ADTs.
2. Select BOTH in Auto Texture.
3. Use the same four texture roles and ranges across both.
4. Manually Apply Auto Texture to both first.
5. Enable Live Auto.
6. Sculpt across their shared ADT edge.
7. Finish the terrain stroke.
8. Confirm both selected loaded sides refresh where expected.
9. Inspect the shared edge for:
   - sudden texture discontinuity;
   - repeated/noise reset patterns;
   - mismatched blend weights;
   - cliff behavior changing abruptly at the border.
10. Press Ctrl+Z once and confirm terrain + textures revert across the affected area.
11. Press Ctrl+Shift+Z once and confirm both return.
12. Save/reload the ADTs and inspect again.
13. If possible, inspect the result in the WoW client too.

## Priority Test 6 — Selected / Unselected ADT Boundary

1. Load two adjacent ADTs.
2. Select only ONE of them in Auto Texture.
3. Enable Live Auto.
4. Sculpt near their shared edge.
5. Confirm Live Auto does NOT replace the texture palette in the unselected ADT.
6. Report whether any skipped-neighbor status is understandable and non-disruptive.

Live Auto must remain constrained to explicitly selected Auto Texture ADTs.

## Priority Test 7 — Auto Texture PASS1B UI Regression

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

## Priority Test 8 — Auto Texture Single ADT Regression

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

## Priority Test 9 — 3 to 8 Selected ADTs

After two-ADT testing succeeds:

1. Try 3, 4, and eventually up to 8 loaded selected ADTs.
2. Confirm manual Apply affects only the selected set.
3. Confirm an unloaded selected ADT causes a safe refusal rather than a partial manual Apply.
4. Exercise Live Auto in the middle and at the edges of the selected set.
5. Confirm Live Auto does not silently auto-load neighboring ADTs.
6. Confirm undo/redo remains one expected transaction for each terrain stroke.

## Live Auto Negative / Safety Regression

Confirm Live Auto does NOT unexpectedly fire during:

- selected-vertex manipulation;
- Script terrain mode;
- Clear Height;
- heightmap import;
- Fix Terrain Gaps;
- Eraser / Clear Tool;
- Chunk Mover;
- Saved Chunk paste/rotation;
- standalone Brush Stack / Stamp Mode mixed work;
- ordinary texture painting.

If any of these trigger Live Auto, report the exact tool, modifiers, map/ADT, and steps.

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

**Live Auto enabled:** Yes / No

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

A report saying “it broke” is difficult to use. A report saying “on Azeroth_24_53, Live Auto retextures the raised chunk but not the east neighbor after releasing Shift+LMB, reproducible after restart” is extremely useful.

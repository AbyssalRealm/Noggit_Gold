# Noggit_Gold — Water + Saved Chunks PASS2

Date: 2026-08-24

## Status

**STATIC-VALIDATED / CURRENT TEST BASELINE**

This package is not COMPILE-PROVEN or LIVE-PROVEN until built and tested on the user's Windows/Noggit environment.
The local configuration check reached dependency population, then stopped because this environment cannot resolve the external GitLab Lua dependency. No gameplay/editor claim is inferred from that failure.

## PASS2A — Water + Gold icon

### Purpose

Preserve the current Noggit_Gold liquid editing engine while adding the Green-reference behavior **Cursor intersect liquids**.
When Water mode is active and the toggle is enabled, cursor height resolves against the visible liquid mesh when that liquid hit is closer than terrain underneath it.

### Added behavior

- Water panel checkbox: **Cursor intersect liquids** (default ON).
- Liquid-layer ray/triangle intersection.
- Tile-level and World-level nearest visible-liquid hit query.
- Water-mode cursor chooses nearest liquid surface without adding liquid objects to the general Noggit selection variant.
- Toggle OFF restores the existing terrain-driven cursor behavior.
- Existing flat-stroke water height and **Follow terrain height** behavior are preserved.

### Gold icon

Replaced the application icon assets with the approved ornate golden elixir bottle:

- `media/noggit_icon.png`
- `media/noggit.ico`

The C++ namespace, QSettings identity, and inherited internal configuration identity remain unchanged.

## PASS2B — Persistent Saved Chunk Library

### Purpose

Extend the LIVE-PROVEN Chunk Manipulator rather than replace it. A copied chunk/group can be saved to disk, selected later like a reusable placement asset, loaded after restarting Noggit, rotated in quarter turns, and pasted on the same or another map through the existing ChunkClipboard paste/action path.

### Persistent location

Saved assets are stored per active Noggit project in:

`<ProjectPath>/saved_chunks/`

Format:

- extension: `.ngchunk`
- magic: `NGCHUNK`
- format version: `1`
- atomic writes through `QSaveFile`

### Automatic naming

The suggested name records zone, source ADT and source chunk, for example:

`Azshara-34_29-c07_12.ngchunk`

Multi-chunk selections append a chunk count, for example:

`Azshara-34_29-c07_12-4chunks.ngchunk`

The user may edit the suggested name before saving. Duplicate names are preserved safely with `_2`, `_3`, etc.

### Saved data

The version-1 payload preserves copied components present in the existing ChunkClipboard cache:

- terrain vertices;
- terrain normals;
- vertex colors;
- shadows;
- liquids and MH2O attributes;
- texture paths/layers/alpha maps/flags/effects/doodad mapping/exclusion;
- WMO and M2 placement data;
- sound emitters;
- holes;
- chunk flags;
- Area ID;
- source/relative placement metadata and original copy flags.

### UI / controls

Chunk Manipulator now has a **Saved Chunks** group with:

- persistent asset list;
- name filter/search box;
- **Save current copy**;
- **Delete saved chunk**;
- **Refresh**.

Selecting a saved entry loads it into the existing placement clipboard.

Transform controls expose:

- **Rotate Left 90°**;
- **Rotate Right 90°**;
- existing Mirror X / Mirror Z for normal live clipboard copies.

When a persistent saved asset is loaded, Mirror X/Z are disabled deliberately. Saved assets support quarter turns only: 0°, 90°, 180°, 270°.

Deletion removes only the `.ngchunk` library file. Terrain already pasted into an ADT is never touched.

## ADD

- `src/noggit/ui/tools/ChunkManipulator/SavedChunkAsset.hpp`
- `src/noggit/ui/tools/ChunkManipulator/SavedChunkAsset.cpp`
- `NOGGIT_GOLD_WATER_SAVED_CHUNKS_PASS2.md`

## REPLACE / MODIFY

- `CMakeLists.txt` — packaged revision label only (`gold-pass2`).
- `media/noggit_icon.png`
- `media/noggit.ico`
- `src/noggit/MapView.cpp`
- `src/noggit/TileWater.hpp`
- `src/noggit/TileWater.cpp`
- `src/noggit/World.h`
- `src/noggit/World.cpp`
- `src/noggit/liquid_layer.hpp`
- `src/noggit/liquid_layer.cpp`
- `src/noggit/tools/WaterTool.hpp`
- `src/noggit/tools/WaterTool.cpp`
- `src/noggit/ui/Water.h`
- `src/noggit/ui/Water.cpp`
- `src/noggit/ui/tools/ChunkManipulator/ChunkClipboard.hpp`
- `src/noggit/ui/tools/ChunkManipulator/ChunkClipboard.cpp`
- `src/noggit/ui/tools/ChunkManipulator/ChunkManipulatorPanel.hpp`
- `src/noggit/ui/tools/ChunkManipulator/ChunkManipulatorPanel.cpp`
- `src/noggit/tools/ChunkTool.hpp`
- `src/noggit/tools/ChunkTool.cpp`

## PROTECTED / UNCHANGED

The following proven paths were not modified by PASS2:

- Eraser / Clear Tool implementation and UI;
- `Action.hpp/.cpp`;
- `ActionManager.hpp/.cpp`;
- terrain sculpting implementation;
- texture-painting implementation;
- M2/WMO Asset Browser / palette implementation;
- the existing ChunkClipboard paste action/undo architecture remains the placement authority.

The Chunk Mover itself is extended only where required for persistence/left rotation/state; existing right-rotation, mirror, paste, height offset, seam repair, object-placement, and action-history behavior remain in place.

## Build

Use the same Windows / Visual Studio / CMake setup that compiled the LIVE-PROVEN Noggit_Gold PASS1 source.

1. Configure/generate if required so the new `SavedChunkAsset.cpp` is discovered. The project uses recursive `CONFIGURE_DEPENDS` source collection, but a fresh generate is the safest first PASS2 build.
2. Build `ALL_BUILD`.
3. Build `INSTALL`.
4. Launch the newly built `Noggit_Gold.exe`.

## LIVE TEST — PASS2A WATER

Use a backed-up/disposable map first.

1. Confirm the new Gold bottle application icon/title presentation.
2. Enter Water mode and confirm **Cursor intersect liquids** exists and starts enabled.
3. Find existing water over clearly uneven/deep terrain.
4. Move the cursor across the water and confirm the editing cursor rides the visible liquid surface rather than terrain below it.
5. Paint/add/remove water and verify normal Water behavior.
6. Verify flat-stroke water height remains stable.
7. Verify **Follow terrain height** still behaves as before.
8. Disable **Cursor intersect liquids** and confirm the old terrain-hit behavior returns.
9. Re-enable it and test multiple liquid heights/layers if available.
10. Confirm Ctrl+Z/Ctrl+Y still work for actual Water edits.

## LIVE TEST — PASS2B SAVED CHUNKS

### Single chunk

1. Select one distinctive chunk with all copy components enabled.
2. Copy it.
3. Click **Save current copy**.
4. Confirm the suggested name resembles `Zone-ADT_X_Z-cXX_ZZ`.
5. Save it and verify it appears in **Saved Chunks**.
6. Select the saved entry; confirm it becomes the placement target.
7. Confirm Mirror X/Z are disabled for the saved asset.
8. Rotate Right 90° and paste.
9. Ctrl+Z; verify complete restoration.
10. Ctrl+Y; verify paste returns.
11. Rotate another quarter turn (180° total), paste elsewhere, verify orientation.
12. Test Rotate Left 90°.

### Persistence / cross-map

1. Close Noggit completely.
2. Reopen the same Noggit project.
3. Confirm the saved entry is still listed.
4. Load the saved entry and paste it successfully.
5. Open another map in the same project.
6. Load the same saved entry and paste it there.
7. Verify terrain, textures, liquids, objects, holes, flags, vertex colors, shadows, Area ID, and sound emitters according to the components that were saved.
8. Undo/redo the cross-map paste.

### Multi-chunk group

1. Select/copy several adjacent chunks.
2. Save the group.
3. Restart Noggit.
4. Load the group.
5. Rotate it 90°/180° and confirm relative chunk layout and all included data remain coherent.
6. Paste and undo/redo.

### Delete

1. Paste a saved asset into a disposable location.
2. Select its library entry and choose **Delete saved chunk**.
3. Confirm the `.ngchunk` disappears from the library.
4. Confirm terrain already pasted into the map is unchanged.
5. If the deleted asset was the active placement clipboard, confirm its target/clipboard clears.

## Rollback

Restore the prior LIVE-PROVEN `Noggit_Gold_Eraser_Clear_Tool_PASS1` source/package. Saved `.ngchunk` files are standalone project-library files and do not modify source or existing ADTs until pasted.

## Proof boundary

- PASS1 Eraser/Clear and Chunk Mover rotation remain the user's LIVE-PROVEN baseline.
- PASS2 source/package integrity and architecture are static-reviewed here.
- Local CMake configuration reached dependency population but could not fetch the external Lua dependency because this execution environment has no external network resolution.
- PASS2 is **not COMPILE-PROVEN** until the user's Windows build succeeds.
- PASS2 is **not LIVE-PROVEN** until the exact Water and Saved Chunk tests above are confirmed by the user.

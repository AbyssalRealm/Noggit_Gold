# Abyssal Realm Noggit_Gold — Eraser / Clear Tool PASS1

Date: 2026-08-24

## Status

CURRENT TEST BASELINE / STATIC-VALIDATED.

This pass ports the selected Noggit Green Clear Tool behavior into the current merged Noggit Red production source and establishes the user-facing **Noggit_Gold** build identity while preserving Red's current Tool architecture and ActionManager undo/redo system.

It is **not LIVE-PROVEN** until the exact checkpoints below are tested in the user's Windows build and map workspace.

## Source authority / provenance

- Production base: current uploaded `noggit-red-master-green-chunk-manipulator-merged` package.
- Red base recorded by the package: `602098c37fdfe3120bac57a7e493744a11cde2fe`.
- Green behavior donor recorded by the package: `59e58add868608339fe292970196985d4ce050ff`.
- Green was used as behavior/reference only. The Eraser was adapted to Red's current `Tool`, `ActionManager`, map/chunk, texture, liquid, and object-storage architecture.
- Existing GPLv3 source headers are retained on added source files.


## Noggit_Gold branding boundary

The public/user-facing build identity is now **Noggit_Gold**:

- Windows executable output: `Noggit_Gold.exe`
- main editor window title: `Noggit_Gold - <version>`
- Qt application display name: `Noggit_Gold`
- Windows CPack bundle/package name: `Noggit_Gold`

Compatibility-sensitive inherited internals remain unchanged on purpose:

- CMake target name remains `noggit`;
- C++ namespace remains `Noggit::`;
- Qt application/organization settings identity remains `Noggit` so existing QSettings continue to resolve;
- configuration keys, file formats, source headers, upstream provenance, and GPLv3 licensing are not mass-renamed.

## Implemented behavior

New toolbar tool: **Eraser / Clear Tool**.

Controls:

- Shift + Left Mouse: clear the selected data.
- Alt + Left Mouse drag: change radius.
- Ctrl+Z: undo the current clearing stroke.
- Ctrl+Y: redo where Red's existing action history provides redo.

Modes:

- Chunk — affects chunks touched by the radius.
- ADT — affects ADTs touched by the radius.

Selective clear options:

- Height
- Textures
- Texture duplicates
- Textures below alpha threshold
- Texture flags
- Liquids
- M2s
- WMOs
- Shadows
- Vertex colors
- Impassible flag
- Holes

Parameters:

- Radius: 0–1000 units, default 15.
- Texture alpha threshold: 0–255, default 1.

Safety behavior:

- No destructive option is enabled by default.
- Clear requires Shift + LMB.
- Radius drag requires Alt + LMB without Shift/Ctrl, preventing radius adjustment from clearing at the same time.
- Each Shift+LMB stroke uses Red's existing ActionManager modality grouping so a continuous stroke is one undo action.

## Red undo integration

The port uses Red's existing action caches instead of Green's older direct-destructive architecture:

- Height -> `registerChunkTerrainChange`
- Textures / alpha / texture flags -> `registerChunkTextureChange`
- Liquids -> `registerChunkLiquidChange`
- M2/WMO removals -> Red action-aware object deletion (`registerObjectRemoved` through existing storage path)
- Shadows -> `registerChunkShadowChange`
- Vertex colors -> `registerChunkVertexColorChange`
- Impassible -> `registerChunkFlagChange`
- Holes -> `registerChunkHoleChange`

No ActionManager or Action implementation file was modified.

## Green donor corrections/adaptations

1. Green already had a backend `clear_texture_flags` option but its Clear Tool panel did not expose a Texture Flags checkbox. Red PASS1 exposes it.
2. Red's normal `eraseUnusedTextures()` behavior and its global cleanup preference remain unchanged. A separate threshold overload was added for this tool instead of changing Red's existing cleanup semantics.
3. Temporary editing alpha maps are normalized from the UI's 0–255 threshold before comparison.
4. Vertex-color clearing resets an existing MCCV set to default white while preserving its presence state. This allows Red's current vertex-color undo cache to restore the exact previous color values without changing Action internals.
5. Liquid clearing uses a narrow `ChunkWater::clearLayers()` operation. Existing water painting, Follow Terrain Height, Chunk Mover liquid relocation, and Water Tool code are unchanged.
6. Chunk-mode M2/WMO deletion follows Green's affected-chunk radius behavior while deletion itself goes through Red's action-aware object removal path.

## Editing-mode protection

Existing values are preserved:

- Chunk Manipulator = 13
- Area Trigger = 14

New value appended only:

- Eraser / Clear Tool = 15

The tool vector appends the new tool after Area Trigger. No existing mode was renumbered.

## ADD

- `src/noggit/tools/ClearingTool.hpp`
- `src/noggit/tools/ClearingTool.cpp`
- `src/noggit/ui/tools/ClearingTool/ClearingToolPanel.hpp`
- `src/noggit/ui/tools/ClearingTool/ClearingToolPanel.cpp`
- `ABYSSAL_ERASER_CLEAR_TOOL_PASS1.md`

## REPLACE / modified source

- `src/noggit/tool_enums.hpp`
- `src/noggit/MapView.cpp`
- `src/noggit/World.h`
- `src/noggit/World.cpp`
- `src/noggit/MapChunk.h`
- `src/noggit/MapChunk.cpp`
- `src/noggit/texture_set.hpp`
- `src/noggit/texture_set.cpp`
- `src/noggit/ChunkWater.hpp`
- `src/noggit/ChunkWater.cpp`

## Explicitly UNCHANGED / protected

- `src/noggit/Action.hpp`
- `src/noggit/Action.cpp`
- `src/noggit/ActionManager.hpp`
- `src/noggit/ActionManager.cpp`
- `src/noggit/tools/ChunkTool.hpp`
- `src/noggit/tools/ChunkTool.cpp`
- `src/noggit/ui/tools/ChunkManipulator/*`
- Chunk Clipboard behavior
- Chunk Mover rotation/mirror behavior
- Chunk Mover liquid relocation
- Red terrain sculpting tools
- Red texture painter
- Red Water Tool painting/height behavior
- Red Light Editor

## Build target

This package is a full source package.

Normal Windows workflow from the project's README:

1. Configure/generate with the same working CMake/Qt/dependency settings used for the current merged Red build.
2. Open the generated Visual Studio solution.
3. Build `ALL_BUILD`.
4. Build `INSTALL` if that is how the current Noggit install directory is populated.
5. Confirm the newly built `Noggit_Gold.exe` timestamp/location before testing.

No server/worldserver build is involved.

## Restart requirements

- Close the old Noggit instance.
- Launch the newly built Noggit executable.

## Client patch requirements

None. This is editor source only.

## Live-test checkpoints

Use a disposable/test ADT or a backed-up custom map first.

### Tool/UI smoke test

1. Launch Noggit.
2. Confirm the new **Eraser / Clear Tool** toolbar entry opens its panel.
3. Confirm no clear checkboxes are selected by default.
4. Confirm Chunk mode is the default.
5. Alt+LMB drag changes only radius and does not clear.
6. Shift+LMB with no options checked does nothing.

### Chunk mode — test individually

For each item below, make a known visible test state, clear it with one Shift+LMB stroke, then immediately Ctrl+Z and verify exact restoration. Ctrl+Y may then be used to verify redo.

1. Height
2. Textures
3. Texture duplicates
4. Textures below threshold
5. Texture flags
6. Liquids
7. M2s only
8. WMOs only
9. M2s + WMOs together
10. Shadows
11. Vertex colors
12. Impassible flag
13. Holes

### Combined operation

1. Select several clear categories at once.
2. Perform one continuous Shift+LMB stroke across multiple chunks.
3. Press Ctrl+Z once.
4. Confirm the entire stroke restores as one action.

### ADT mode

1. Select ADT mode.
2. Clear one category on one ADT and undo it.
3. Test M2 only, WMO only, then both on an ADT and undo each.
4. Increase radius enough to touch neighboring ADTs and confirm only the intended ADTs are affected.
5. Undo and verify all affected ADTs restore.

### Save/reopen proof

1. Perform an intentional clear operation and leave it applied.
2. Save the map.
3. Close/reopen Noggit.
4. Verify the saved cleared state persists correctly.

### Protected Chunk Manipulator regression

After Eraser testing:

1. Copy a known chunk selection with the existing Chunk Manipulator.
2. Paste it.
3. Undo the paste.
4. Repeat with terrain + textures + liquid selected.
5. Confirm rotation/mirror and normal copy/paste still behave as before.

Do not reopen or redesign the Chunk Manipulator unless this regression reproduces a defect.

## Proof boundary / validation performed here

- `git diff --check`: passed.
- Confirmed Chunk Manipulator files are untouched.
- Confirmed existing editing modes 0–14 retain their values and the Eraser is appended as mode 15.
- Confirmed CMake recursively collects new `.cpp`/header files under `src/noggit`.
- CMake configuration was attempted in the analysis environment. It reached dependency population but could not download the project's Lua dependency because external network/DNS access is unavailable in that environment. Therefore this package is **not COMPILE-PROVEN here**.
- No in-game/editor-runtime claim is made until the user's local Visual Studio build and live test.

## Rollback

Clean rollback is to restore the original uploaded merged Red source package.

For a narrow source rollback, remove the four new ClearingTool files and restore the ten modified source files listed under REPLACE from the original merged Red package. The protected Chunk Manipulator and ActionManager files require no rollback because they were not changed.

## Next gate

Do not start the Water Tool port until this Eraser pass compiles and the user proves its destructive operations plus Ctrl+Z behavior in Noggit.

## Deferred Chunk Mover upgrade — after Eraser / Water / Light Editor

Do **not** implement this during PASS1. After the current editor tools are proven, perform a fresh-source audit for a controlled Chunk Mover upgrade with these requirements:

- Copy a chunk and optionally save it as a persistent reusable chunk asset/preset.
- Saved chunks survive Noggit close/relaunch and can be reused on the same map or another map.
- Provide a palette/library selection workflow comparable to choosing a reusable M2/WMO asset for placement.
- Placement supports quarter-turn rotation only: 0°, 90°, 180°, and 270° with simple rotate-left / rotate-right controls.
- No mirror, flip, arbitrary deformation, or other unusual transforms in this saved-asset workflow.
- Support preview/rotate-before-paste where practical; after placement, the normal selected-chunk rotation workflow may also be used.
- Preserve the current LIVE-PROVEN Chunk Mover behavior, liquid relocation, and Red Ctrl+Z/Ctrl+Y action history.
- Cross-map assets must store the complete data actually required to reproduce the copied chunk safely; exact serialization format and dependency handling remain undecided until the fresh audit.


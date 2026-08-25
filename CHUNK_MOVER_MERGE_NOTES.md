# Chunk Manipulator merge notes

## Direction and source identities

- **Target kept intact:** `noggit-red-master`, commit `602098c37fdfe3120bac57a7e493744a11cde2fe`.
- **Behavior donor:** `noggit3-default` (GREEN), commit `59e58add868608339fe292970196985d4ce050ff`.
- The GREEN chunk mover was not transplanted as an old subsystem. Its behavior was mapped onto RED's C++20 tool, renderer, GLM, texture, liquid, object, and undo APIs.

## Exact merge map

| GREEN behavior | RED implementation |
| --- | --- |
| Chunk selection and cached chunk data | `ChunkClipboard.hpp/.cpp` |
| Copy/paste of terrain, textures, alpha maps, water, objects, shadows, vertex colors, holes, flags, area IDs, and sound emitters | `ChunkClipboard.cpp` |
| Rotate 90 degrees and mirror X/Z | `ChunkClipboard.cpp` plus private liquid-layer transforms |
| Water relocation into another chunk | `liquid_layer.*` and `ChunkWater.*` |
| Source and target overlays | `MapChunk.*`, `TileRender.cpp`, and `terrain_frag.glsl` |
| Chunk-mover controls and options | `ChunkManipulatorPanel.*` |
| Tool lifecycle, mouse selection, Ctrl+C, and Ctrl+V | `ChunkTool.*` using RED's existing global copy/paste hotkey routes |
| Undo/redo for all copied chunk fields, including liquid attributes and sound emitters | `Action.*` and `ActionHistoryNavigator.cpp` |
| Stable flat liquid painting plus optional terrain-following liquid slopes | `WaterTool.*`, `ui/Water.*`, `World.*`, `ChunkWater.*`, and `liquid_layer.*` |

## Controls

- Hold **Shift + left mouse** to add chunks to the source selection.
- Hold **Ctrl + left mouse** to remove chunks from the source selection.
- Hold **Alt + left mouse** and drag horizontally to change the selection radius.
- Press **Ctrl+C** to copy the selected chunks with the enabled component filters.
- Move the cursor to preview the target in green, then press **Ctrl+V** to paste.
- Use the panel buttons to rotate 90 degrees, mirror X, mirror Z, or clear the selection.
- The source selection is blue. The paste target is green.
- Water painting defaults to one stable height per paint stroke. Enable **Follow terrain height** to shape each painted liquid vertex to its matching terrain vertex with a 1-unit clearance; this mode takes priority over Lock and Angled mode.
- **Height** under Override must be enabled when reshaping liquid that already exists.

## Verification status

- The source-to-target diff is limited to the chunk-transfer implementation and this note.
- Whitespace/error-marker checks pass.
- Rotation/mirroring lookup and bit-mask invariants pass a standalone C++20 smoke test (four rotations and double mirrors return the original data).
- Flat-stroke height and terrain-to-liquid 17×17-to-9×9 vertex-mapping invariants pass a standalone C++20 smoke test.
- This corrected package is based on RED commit `602098c37fdfe3120bac57a7e493744a11cde2fe` and includes the contents of its pinned `cmake`, `dist`, Blizzard archive-library, Blizzard database-library, and nested dependency submodules.
- The top-level revision target falls back to the packaged RED revision when `.git` metadata is absent, so the normal source ZIP builds without requiring a local Git repository.
- The validation environment does not provide a CMake executable or Qt development headers, so a full configure/link run was not possible here. Package-integrity, dependency-presence, diff, and standalone transform checks pass.

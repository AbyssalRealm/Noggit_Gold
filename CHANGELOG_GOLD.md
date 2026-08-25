# Noggit_Gold Changelog

## Public Test Candidate — 2026-08-25

### Live Auto Texture PASS1 — SEMI-LIVE-PROVEN

**Status:** SEMI-LIVE-PROVEN — awaiting more hands to break things.

Initial live testing passed on the current author build. Broader public proof across more maps, machines, brush sizes, chunk boundaries, adjacent ADTs, save/reload cycles, and game-client inspection is still requested before this is promoted to the fully live-proven baseline.

Source introduction commit:

`79944bbb86e7fa191a29f8d7251c2cf292355c29` — **Add Live Auto Texture terrain reapply PASS1**

Implemented:

- Added opt-in `Reapply after terrain edits` control in Texture Painter -> Auto.
- Live Auto defaults OFF.
- Added an explicit first-pass Live Auto eligibility action flag rather than triggering from every terrain action.
- Raise / Lower normal terrain sculpt paths can trigger Live Auto.
- Flatten / Blur normal terrain sculpt paths can trigger Live Auto.
- Normal image-mask terrain sculpting through Raise / Lower can trigger Live Auto.
- Selected-vertex and Script terrain modes remain excluded from the first pass.
- Added Action readback of the exact registered changed-terrain chunk set.
- Added ActionManager `onActionAboutToFinish` hook before Action post-state capture in both normal action-end and modality-mismatch action-end paths.
- Live Auto runs once when the eligible terrain action is finishing rather than continuously every frame.
- Reuses the existing Auto Texture terrain height / triangle slope / world-space-noise evaluator.
- Reuses the existing four-role Base / Low Ground / High Ground / Cliff texture model.
- Reuses the existing temporary-alpha / texture-set application path.
- Reapplies only the affected terrain chunks plus one loaded neighbor-chunk ring where allowed.
- Constrains Live Auto to explicitly selected Auto Texture ADTs.
- Does not auto-load neighboring ADTs.
- Skips actions that already contain unrelated texture edits in the first pass.
- Terrain and generated texture changes are captured inside the same ActionManager action path for one-step undo/redo.

Still requiring broader proof:

- one-step terrain + generated-texture undo/redo across more machines and maps;
- internal chunk-edge behavior;
- adjacent-ADT Live Auto seam behavior;
- selected/unselected ADT boundary behavior;
- wider 3–8 ADT Live Auto use;
- save/reload persistence and in-game client appearance;
- performance under large brushes / long terrain strokes;
- regression against excluded terrain/bulk/manipulation paths.

### Auto Texture PASS1 / PASS1B

- Added explicit selected-ADT Auto Texture workflow.
- Maximum 8 selected ADTs per Apply.
- Requires selected ADTs to already be loaded; refuses unsafe partial processing.
- Four texture roles: Base, Low Ground, High Ground, Cliff.
- Uses terrain height and surface/triangle slope independently.
- Uses the existing temporary-alpha / texture-set / MCAL application path.
- Uses world-space boundary variation to avoid chunk/ADT-local noise resets.
- Fit Heights scans the current terrain vertices.
- Auto Texture Apply is one normal ActionManager transaction.
- Added selected terrain Min / Max / Span feedback.
- Added Low/High band intersection feedback.
- Improved narrow-panel layout.
- Added slot-specific access to the existing Texture Browser while retaining `Use Current`.
- Four-slot Texture Browser assignment path is live-tested successfully.

### Previously integrated/live-tested Gold work

- Eraser / Clear Tool selective clearing and undo/redo.
- Water liquid-surface cursor intersection.
- Chunk Mover copy/paste and quarter-turn rotation.
- Persistent Saved Chunks and tested object/asset carrying.
- Light Editor Port to Light and selected-light visibility path.
- Noggit_Gold branding/icon work.

### Still requiring broader proof

- Live Auto Texture PASS1 across broader public testing.
- Adjacent-ADT Auto Texture seams.
- Wider 3–8 ADT Auto Texture use.
- Saved/reloaded WoW-client appearance across varied maps.
- Light Editor DBC write/restart persistence lifecycle.

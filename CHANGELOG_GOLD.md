# Noggit_Gold Changelog

## Public Test Candidate — 2026-08-25

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

### Previously integrated/live-tested Gold work

- Eraser / Clear Tool selective clearing and undo/redo.
- Water liquid-surface cursor intersection.
- Chunk Mover copy/paste and quarter-turn rotation.
- Persistent Saved Chunks and tested object/asset carrying.
- Light Editor Port to Light and selected-light visibility path.
- Noggit_Gold branding/icon work.

### Still requiring broader proof

- Adjacent-ADT Auto Texture seams.
- Saved/reloaded WoW-client appearance across varied maps.
- Light Editor DBC write/restart persistence lifecycle.

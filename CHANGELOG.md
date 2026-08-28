# Noggit_Gold Changelog

This changelog records user-facing Noggit_Gold changes. Internal development notes, test scratchpads, and implementation pass logs are intentionally not kept in the public repository.

## Unreleased

### Documentation and repository cleanup

- Added a dedicated installation guide for ready-to-run Windows builds and source builds.
- Consolidated public change history into this changelog.
- Removed internal development/pass notes and the repository push helper script from the public tree.
- Simplified public documentation around installation, usage, changes, licensing, and reproducible bug reports.
- Updated public credits at contributor request while retaining inherited Git history and license information.

## v1.1 — 2026-08-25

### Minimap Generator

- Fixed tile-to-tile minimap rendering state/order behavior.
- Verified adjacent Azeroth minimap tiles with repeatable generation order.
- Preserved minimap export and `md5translate` handling.

### Texture Painter

- Compacted the Paint tab layout.
- Corrected excessive vertical spacing around the Opacity control.
- Tightened Hardness, Radius, and Pressure control spacing.
- Widened numeric value fields for large brush values.
- Preserved existing painting ranges and behavior.

### Auto Texture

- Added selected-ADT Auto Texture processing.
- Added four texture roles: Base, Low Ground, High Ground, and Cliff.
- Added terrain-height and slope-aware texture distribution.
- Added world-space boundary variation for smoother transitions.
- Added selected-terrain minimum, maximum, and span feedback.
- Added Fit Heights assistance.
- Added direct texture-slot selection through the existing Texture Browser.
- Added opt-in terrain-edit reapplication for supported sculpt actions.
- Kept generated terrain and texture changes on the normal undo/redo action path.

### Water Tool

- Improved cursor placement against the visible liquid surface.

### Chunk Mover and Saved Chunks

- Added tested quarter-turn rotation support.
- Added persistent Saved Chunks for reusable terrain sections.
- Preserved associated placed objects/assets in tested saved-section workflows.

### Light Editor

- Added Port to Light navigation for selected local lights.
- Improved selected-light inspection through the existing current-only/wireframe workflow.

### Branding

- Updated the editor branding and icon presentation for Noggit_Gold.

## Initial Noggit_Gold public test — 2026-08-25

- Published the first Windows x64 public test build.
- Established the current Noggit_Gold branch as a maintained WotLK 3.3.5 map-authoring continuation.
- Began public testing of the Gold terrain, texture, liquid, lighting, chunk, object, and persistence improvements.

## Project lineage

Noggit_Gold preserves the inherited Noggit source history, licensing, notices, and contributor history in Git. See the repository history and `COPYING` for the authoritative inherited record.
# Noggit_Gold

**Noggit_Gold** is a WotLK 3.3.5 map-authoring continuation built from the merged Noggit Red production source, with selected useful behavior adapted from earlier Noggit branches while preserving Red/Gold's newer architecture.

This repository is open for **public testing**. The goal is not to claim every editor path is finished; it is to get the current working WotLK tool into more hands so terrain, texture, liquid, lighting, chunk, object, and persistence edge cases can be tested across many real maps and systems.

## Public Test Status

### Live-proven in the current Gold baseline

- Eraser / Clear Tool across the tested clear categories.
- Eraser undo/redo through Noggit's existing ActionManager path.
- Chunk Mover copy/paste.
- Chunk Mover quarter-turn rotation, including tested 90-degree and 180-degree placements.
- Water Tool cursor intersection with the visible liquid surface.
- Persistent Saved Chunks core behavior.
- Saved multi-chunk sections carrying associated objects/assets in tested cases.
- Saved-section quarter-turn rotation and paste.
- Light Editor **Port to Light** navigation for selected local lights.
- Selected-light identification using the existing Draw Current Only / wireframe path.
- Auto Texture PASS1 single-ADT application.
- Auto Texture terrain-aware elevation/slope distribution.
- Auto Texture cliff texturing on steep terrain.
- Auto Texture undo/redo as one normal ActionManager transaction.
- Auto Texture PASS1B selected-terrain Min / Max / Span feedback and Fit Heights workflow.
- Auto Texture Base / Low Ground / High Ground / Cliff slot selection through the existing Texture Browser.

### SEMI-LIVE-PROVEN — awaiting more hands to break things

**Live Auto Texture PASS1** is now in `main`.

Initial live testing passed on the current author build. Broader public testing is deliberately still required before this is promoted to the fully live-proven baseline.

Live Auto adds an opt-in **Reapply after terrain edits** mode to the existing Auto Texture panel. When enabled, eligible terrain sculpt actions re-run the same existing terrain-aware Auto Texture evaluator after the terrain action finishes.

Current first-pass behavior:

- eligible Raise / Lower terrain sculpting can trigger Live Auto;
- Flatten and Blur can trigger Live Auto;
- normal image-mask terrain sculpting through the Raise / Lower tool can trigger Live Auto;
- selected-vertex and Script terrain modes are excluded from the first pass;
- the changed terrain chunks are used as the source work set;
- one loaded neighbor-chunk ring is included where it remains inside selected Auto Texture ADTs;
- Live Auto does not auto-load neighboring ADTs;
- Live Auto does not run outside the explicitly selected Auto Texture ADT set;
- terrain and generated texture state are captured through the same ActionManager action path;
- mixed actions that already contain unrelated texture edits are skipped by Live Auto in this first pass;
- the feature defaults OFF and must be explicitly enabled.

The public source commit introducing this pass is:

`79944bbb86e7fa191a29f8d7251c2cf292355c29` — **Add Live Auto Texture terrain reapply PASS1**

Please try to break it on different maps, machines, brush sizes, chunk edges, and adjacent ADTs and report reproducible failures.

### Current public-test focus

The current source includes **Auto Texture PASS1 / PASS1B plus Live Auto Texture PASS1**.

Auto Texture UI/feedback includes:

- selected terrain minimum / maximum / span display;
- Low/High height-band intersection feedback;
- Fit Heights helper;
- tighter vertical layout for narrow dock widths;
- Base / Low Ground / High Ground / Cliff texture slots;
- clicking a texture preview/name opens the existing Noggit Texture Browser for that slot;
- `Use Current` remains available as a shortcut.

The four-slot Texture Browser selection path has been **live-tested successfully**. Live Auto Texture PASS1 has passed its initial author test and is **SEMI-LIVE-PROVEN** while we wait for broader hands-on testing.

Public testing should now concentrate on:

- Live Auto Raise / Lower / Flatten / Blur behavior across different maps and brush sizes;
- one-step terrain + generated-texture undo/redo under Live Auto;
- internal chunk-edge behavior;
- adjacent-ADT Live Auto behavior and seam continuity;
- wider 3–8 selected-ADT use;
- save/reload and in-game appearance;
- Light DBC write persistence;
- general regression across different machines and maps.

## Important Auto Texture Safety Boundary

Auto Texture remains deliberately conservative:

- only explicitly selected ADTs are processed;
- selected ADTs must already be loaded;
- unloaded selected ADTs cause manual Apply to refuse rather than partially write;
- maximum 8 selected ADTs per Apply;
- no implicit Entire Map mode;
- Auto Texture replaces the existing four-layer terrain palette on selected Auto Texture work areas;
- manual Apply is captured through the existing ActionManager undo/redo system;
- Live Auto defaults OFF;
- Live Auto only runs for explicitly eligible first-pass terrain sculpt actions;
- Live Auto only retextures changed chunks plus the permitted loaded neighbor ring inside the selected ADT coverage;
- Live Auto does not auto-load neighboring ADTs;
- Live Auto skips an action that already contains unrelated texture edits.

Back up important map work before testing any editor build.

See **[PUBLIC_TESTING.md](PUBLIC_TESTING.md)** for the exact test checklist and bug-report format.

## Known Unproven / Incomplete Areas

- Live Auto Texture PASS1 needs broader proof across more users, machines, maps, brush sizes, and edge cases.
- Adjacent-ADT Auto Texture and Live Auto seam behavior needs broader proof.
- Wider 3–8 ADT use needs broader proof.
- Full in-game client appearance after save/reload still needs broader proof across maps.
- Light Editor DBC write -> full restart -> persistence lifecycle is not yet public/live-proven and should only be tested against disposable/backed-up DBC copies.
- Light Editor float-curve editing, deep independent light duplication, Delete Light, and Save Name are not considered finished production paths.

## Building

This project uses CMake and Qt5. On Windows, the existing Noggit build expects the usual Noggit dependencies such as OpenGL, StormLib, CascLib, Qt5, and Lua. Some dependencies may be obtained through FetchContent depending on your environment.

Typical Windows flow:

1. Install a compatible Visual Studio C++ toolchain and Qt5.
2. Configure the project with CMake.
3. Set the Qt prefix/path as required by your local install.
4. Generate the Visual Studio solution.
5. Build the desired target in Visual Studio.
6. Ensure the required Qt runtime DLLs are available beside the executable or otherwise discoverable.

The inherited build files and comments remain the authority for platform-specific details; public testers are encouraged to report clean-build issues with exact compiler/CMake/Qt versions.

## Reporting Bugs

Please use GitHub Issues and include:

- exact Noggit_Gold build/commit;
- Windows/Linux version;
- GPU and driver if the issue is graphical;
- map and ADT coordinates;
- exact tool and steps;
- whether Live Auto was enabled;
- whether undo/redo was involved;
- whether the ADT was saved/reloaded;
- whether the result was also checked in the WoW client;
- screenshots or a short video when useful;
- crash log/stack trace when available.

Do **not** upload proprietary Blizzard game data to this repository when reporting a bug.

## Project Lineage and Credits

Noggit_Gold is **not** presented as a from-scratch editor. It continues the existing Noggit lineage and preserves inherited source history, licenses, notices, and contributor work.

The retained project lineage includes:

- **Noggit / Noggit3** — original upstream project and contributors;
- **Skarn** — creator and original developer of **Noggit Red**;
- **Titi / T1ti, Varenroth, RussianBias, and other Noggit Red contributors** — later Red development, features, fixes, and branch/fork work;
- **Marlamin** — maintained a separate Noggit Red aggregation fork combining work from the Titi, Varenroth, and RussianBias forks, with additional changes;
- **Noggit Green** — selected useful behavior adapted into the current Red/Gold production architecture where present;
- **Noggit_Gold / Abyssal Realm** — current integration, maintenance, fixes, and new Gold additions/adaptations.

The retained Git history and upstream notices remain the detailed attribution authority. If an inherited contribution is missing or described incorrectly, please report the exact source/history reference so the public credits can be corrected.

## License

The inherited project is distributed under **GNU GPL v3**. See [`COPYING`](COPYING).

Keep applicable license notices, copyright statements, upstream attribution, and source availability intact when redistributing modified builds.

## Game Data

Noggit_Gold is an editor. This repository should not bundle proprietary Blizzard game data. Testers are responsible for providing their own compatible game data.

# Noggit_Gold

**Noggit_Gold** is a maintained World of Warcraft **3.3.5a / Wrath of the Lich King** map-authoring continuation built from the Noggit lineage and focused on practical world editing, stability, and a clean public workflow.

The project remains open source and preserves the inherited Noggit history and GPL licensing.

## Features

Current Noggit_Gold work includes:

- terrain editing and normal Noggit world-authoring tools;
- Eraser / Clear Tool with normal undo/redo integration;
- Chunk Mover copy/paste and quarter-turn rotation;
- persistent Saved Chunks for reusable terrain sections;
- tested saved-section object/asset carrying;
- Water Tool cursor placement against the visible liquid surface;
- Light Editor Port to Light navigation and selected-light inspection improvements;
- selected-ADT Auto Texture workflow;
- terrain-height and slope-aware texture distribution;
- Base / Low Ground / High Ground / Cliff Auto Texture roles;
- selected-terrain Min / Max / Span feedback and Fit Heights;
- direct Auto Texture slot selection through the existing Texture Browser;
- opt-in Auto Texture reapplication after supported terrain edits;
- minimap generation/export fixes for consistent adjacent-tile output;
- Noggit_Gold branding and UI cleanup.

See **[CHANGELOG.md](CHANGELOG.md)** for the public change history.

## Installation

For the ready-to-run Windows build and source-build instructions, see **[INSTALL.md](INSTALL.md)**.

The short version for a release build is:

1. Download the latest release from GitHub Releases.
2. Extract it to its own folder.
3. Use a dedicated, backed-up World of Warcraft 3.3.5a client/project workspace.
4. Launch `Noggit_Gold.exe`.
5. Create or select a Noggit project and point it at the compatible client data you intend to edit.

Noggit_Gold does **not** include proprietary Blizzard game data.

## Building

Noggit_Gold uses CMake, C++20, Qt5, and the inherited Noggit dependency stack. Windows development is supported through Visual Studio 2022 and an x64 Release build.

Clone with submodules:

```powershell
git clone --recurse-submodules https://github.com/AbyssalRealm/Noggit_Gold.git
cd Noggit_Gold
```

Then follow **[INSTALL.md](INSTALL.md)** for configuration and build details.

## Safety

Back up important map, ADT, DBC, and project files before editing. Experimental patches, converted assets, and cross-expansion data should be tested in disposable client/project copies before they are used on valuable work.

## Reporting Bugs

Please use GitHub Issues and include enough information to reproduce the problem:

- exact Noggit_Gold release or commit;
- operating system;
- map and ADT coordinates when relevant;
- tool being used;
- exact reproduction steps;
- expected and actual result;
- whether undo/redo or save/reload was involved;
- screenshots, video, or logs when useful.

Do **not** upload proprietary Blizzard game data to the repository.

## Project Lineage and Credits

Noggit_Gold is **not** presented as a from-scratch editor. It continues the existing Noggit lineage and preserves inherited source history, licenses, notices, and contributor work.

The public lineage includes:

- **Noggit / Noggit3** — original upstream project and contributors;
- **Skarn** — creator and original developer of **Noggit Red**;
- **Titi / T1ti, Varenroth, RussianBias, and other Noggit Red contributors** — later Red development, features, fixes, and branch/fork work;
- **Noggit Green** — selected useful behavior adapted into the current Red/Gold architecture where present;
- **Noggit_Gold / Abyssal Realm** — current integration, maintenance, fixes, and Gold additions/adaptations.

The retained Git history and upstream notices remain the detailed attribution record. If inherited attribution is missing or inaccurate, please report the exact source/history reference so it can be corrected.

## License

The inherited project is distributed under **GNU GPL v3**. See [`COPYING`](COPYING).

Keep applicable license notices, copyright statements, upstream attribution, and source availability intact when redistributing modified builds.

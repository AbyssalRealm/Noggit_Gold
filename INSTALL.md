# Installing Noggit_Gold

Noggit_Gold targets **World of Warcraft 3.3.5a (Wrath of the Lich King)** map-authoring workflows.

## Ready-to-run Windows build

1. Download the latest Windows release from the repository's **Releases** page.
2. Extract the archive to its own folder. Do not run the editor directly from the ZIP file.
3. Use a dedicated, backed-up 3.3.5a client or project workspace for editing.
4. Launch `Noggit_Gold.exe`.
5. On first launch, create or select a Noggit project and point it to the compatible 3.3.5a client data you intend to edit.
6. Keep the files supplied with the release beside the executable unless you know they are provided elsewhere on your system.

Back up important ADTs, DBCs, and other project data before editing. Noggit_Gold does not include proprietary Blizzard game data.

## Building from source on Windows

### Requirements

- Visual Studio 2022 with the Desktop development with C++ workload
- CMake 3.11 or newer
- A C++20-capable compiler
- Qt5
- Git

The build also uses the dependencies declared by the project, including OpenGL, Lua, StormLib, CascLib, Json, lodepng, Sol2, and the bundled/submodule external libraries.

Qt5 is required with these components:

- Widgets
- OpenGLExtensions
- Gui
- Network
- Xml
- Multimedia
- Sql

### Clone

Clone the repository with its submodules:

```powershell
git clone --recurse-submodules https://github.com/AbyssalRealm/Noggit_Gold.git
cd Noggit_Gold
```

If the repository was already cloned without submodules:

```powershell
git submodule update --init --recursive
```

### Configure

Create a separate build directory and configure with CMake. Point `CMAKE_PREFIX_PATH` at the Qt5 installation used on your machine.

Example:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64"
```

The Qt path above is only an example; use the path that matches your installation.

### Build

```powershell
cmake --build build --config Release
```

You can also open the generated Visual Studio solution and build the `noggit` target in **Release x64**.

### Runtime files

A source build still needs its runtime dependencies available beside the executable or discoverable through the system environment. In particular, make sure the required Qt5 runtime DLLs and Qt plugins are present.

## First project

Noggit_Gold is intended to work against a compatible 3.3.5a project/client data set. Keep experimental patches and cross-expansion assets in disposable test copies until you know they are compatible with the editor and your target client.

## Updating

For a ready-to-run build, extract a new release into a clean folder rather than overwriting a working installation in place. Keep your project data separate from the editor binaries whenever possible.

For a source checkout:

```powershell
git pull
git submodule update --init --recursive
cmake --build build --config Release
```

If dependencies or CMake configuration changed substantially, regenerate the build directory before compiling.

## Problems

When reporting a reproducible problem, include the Noggit_Gold version or commit, operating system, map/ADT coordinates when relevant, exact steps, and the log or screenshot needed to reproduce it.

Do not upload proprietary Blizzard game data to the repository.
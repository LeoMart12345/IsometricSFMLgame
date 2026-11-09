# IsometricSFMLgame

<img width="536" height="451" alt="image" src="https://github.com/user-attachments/assets/f57ec7b4-5a26-4d64-a69d-4386b79c274b" />

A small Diablo-style isometric map editor built with **C++20** and **SFML 3**.
Left click stamps the selected tile, right click clears a cell, and the map is
layered in three z levels.

Game assets are not mine.

## Requirements

| Dependency | Version | Notes |
|---|---|---|
| C++ compiler | C++20 | GCC 11+, Clang 14+, MSVC 19.30+ (VS 2022) |
| SFML | **3.0** | The code uses the SFML 3 API (scoped enums, `Event::getIf`) |
| CMake | 3.20+ | Optional, only needed for the CMake build |
| Visual Studio | 2022 (v143) | Optional, `IsometricSFMLgame.sln` |

## Building

### CMake (Linux, macOS, Windows)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/SFML-3.0.0
cmake --build build
./build/IsometricGame
```

`find_package(SFML 3 COMPONENTS Graphics Window System)` is used, so either
install SFML 3 system-wide or point `CMAKE_PREFIX_PATH` at an unpacked SDK.
On Windows the SFML DLLs are copied next to the executable automatically.

### Visual Studio

Open `IsometricSFMLgame.sln`, select `x64`, and build. The project reads the
SFML SDK location from the `SFMLDir` property, which defaults to
`C:\ProgrammingProjects\SFML-3.0\SFML-3.0.0`. Override it without touching the
project file:

```bat
msbuild IsometricSFMLgame.sln /p:Configuration=Release /p:Platform=x64 /p:SFMLDir=D:\libs\SFML-3.0.0
```

The project expects an **SFML 3.0** SDK; SFML 2.x will not compile.

### Tests

The isometric projection lives in a header without any SFML dependency, so its
unit tests build and run without a graphics context:

```bash
cmake -S . -B build -DISO_BUILD_GAME=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Controls

| Input | Action |
|---|---|
| `Enter` / `Space` (title screen) | Open the editor |
| `Escape` (title screen) | Quit |
| `Escape` (editor) | Back to the title screen |
| `H` | Raise the active z level (max 2) |
| `G` | Lower the active z level (min 0) |
| `0` | Select "clear" (right click also clears) |
| `1`–`7` | Grass, Forest, Water, Residential, Commercial, Industrial, Road |
| Left click | Place the selected tile on the active z level |
| Right click | Clear the clicked cell |
| Mouse move | Shows a translucent preview of the placement |

## Assets

Art is loaded from `assets/` (see `assets/README.md` for the expected file
names). Missing files are not fatal: the game generates solid-colour diamond
placeholders and prints a warning on the console, so it always starts.

## Project layout

```
IsometricSFMLgame.sln
CMakeLists.txt              CMake build (game + tests)
IsometricSFMLgame/          Visual Studio project
  IsometricSFMLgame.vcxproj
  src/
    main.cpp                Entry point
    core/                   Game shell: window, loop, state stack, input, textures
    states/                 Title screen and map editor states
    map/                    Map, tiles, tile atlas, pure projection math
    ui/                     Reusable widgets (image button)
    animation/              Sprite-sheet animation helpers
tests/
  grid_geometry_test.cpp    Projection/unprojection checks (no SFML needed)
docs/
  ARCHITECTURE.md           How the pieces fit together
```

Include paths are rooted at `src/`, so headers are included as
`#include "map/map.hpp"`.

## Status / limitations

* The editor is a sandbox: maps are not saved or loaded yet.
* `Button` and `AnimationHandler` are implemented and covered by the header
  layout above, but no state uses them yet (the editor has no HUD and tiles are
  not animated).
* The old `tile_preview`, `UIManager` and `SimpleUi` stubs were removed; the
  hover preview now belongs to `Map`.

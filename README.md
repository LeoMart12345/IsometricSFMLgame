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
| SFML | **3.1** (3.0 also works) | The code uses the SFML 3 API (scoped enums, `Event::getIf`, `Rect::position`/`Rect::size`). SFML 2.x will **not** compile |
| CMake | 3.20+ | Optional, only needed for the CMake build |
| Visual Studio | 2022 (v143) | Optional, `IsometricSFMLgame.sln` |

## Building

### CMake (Linux, macOS, Windows)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/SFML-3.1.0
cmake --build build
./build/IsometricGame
```

`find_package(SFML 3 COMPONENTS Graphics Window System)` is used, so either
install SFML 3 system-wide or point `CMAKE_PREFIX_PATH` at an unpacked SDK.
On Windows the SFML DLLs are copied next to the executable automatically.

Configuring also writes `build/compile_commands.json` (unless the sources are
built without CMake), and `.vscode/c_cpp_properties.json` consumes it, so the
editor's IntelliSense uses the same SFML 3 headers as the compiler. Without
that, a system-wide **SFML 2** install in `/usr/include/SFML` is picked up and
the editor reports errors such as *"`sf::Rect<int>` has no member named
`size`"* even though the code compiles: in SFML 3 a rectangle exposes
`position` and `size`, whereas SFML 2 used `left/top/width/height`.

On Linux the graphics modules of SFML also need the X11, OpenGL and UDev
*headers* (`libXrandr`, `libXi`, `libXcursor`, `libXfixes`, `libglvnd`,
`systemd`/`udev`). SFML 3 is not packaged by every distribution yet.

### Linux helper script (no root required)

`scripts/build-linux.sh` does the whole dance and is safe to re-run:

```bash
./scripts/build-linux.sh            # Debug build + tests
./scripts/build-linux.sh --build-type Release --jobs 8
```

It

1. looks for the X11/GL/UDev headers and, when they are missing, extracts the
   Fedora `-devel` RPMs into a private sysroot with `dnf` + `rpm2cpio`
   (**no `sudo`**),
2. builds SFML 3.1 (shared) into a private prefix unless a system SFML 3 is
   already visible to `pkg-config` (override the release with
   `ISO_SFML_VERSION=3.0.0`),
3. configures, builds and runs `ctest` for this project.

Everything it creates lives in `~/.cache/isometric-sfml3` (override with
`ISO_DEPS=...`) and `./build`, so `rm -rf` undoes it completely.

### Running without a display

The game opens a real window, so it needs an X server. On a headless machine
(or over SSH) use a virtual one:

```bash
Xvfb :99 -screen 0 1920x1080x24 &
DISPLAY=:99 ./build/IsometricGame
```

The unit tests never need a display.

### Visual Studio

Open `IsometricSFMLgame.sln`, select `x64`, and build. The project reads the
SFML SDK location from the `SFMLDir` property, which defaults to
`C:\ProgrammingProjects\SFML-3.1\SFML-3.1.0`. Override it without touching the
project file:

```bat
msbuild IsometricSFMLgame.sln /p:Configuration=Release /p:Platform=x64 /p:SFMLDir=D:\libs\SFML-3.1.0
```

The project expects an **SFML 3.x** SDK (3.1.0 is what CI builds against; 3.0
also works); SFML 2.x will not compile.

### Tests

The simulation layer (RNG, tile grid, pathfinding, stats, combat, actors) has no
SFML dependency, so its unit tests build and run without a graphics context:

```bash
cmake -S . -B build -DISO_BUILD_GAME=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Individual suites can be filtered too: `./build/game_logic_test --filter combat`.

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
names). Missing files are not fatal and never throw: the game generates
solid-colour diamond placeholders instead, and reports which files it is
missing in a single block on the console, so it always starts.

```
[assets] 10 of 10 textures have no art file; drawing generated placeholders for them:
           'grass' from 'assets/pxArt.png'
           ...
```

## Project layout

```
IsometricSFMLgame.sln
CMakeLists.txt              CMake build (game + tests)
scripts/
  build-linux.sh            Bootstraps SFML 3 + X11 headers, then builds and tests
.vscode/
  c_cpp_properties.json     IntelliSense/compile_commands setup (SFML 3 include paths)
IsometricSFMLgame/          Visual Studio project
  IsometricSFMLgame.vcxproj
  src/
    main.cpp                Entry point
    core/                   Game shell: window, loop, state stack, input, textures,
                            assertions and the seeded RNG
    game/                   SFML-free simulation: stats, combat, actors, pathfinding
    states/                 Title screen and map editor states
    map/                    Map, tiles, tile atlas, tile grid, pure projection math
    ui/                     Reusable widgets (image button)
    animation/              Sprite-sheet animation helpers
tests/
  test_framework.hpp        Tiny header-only test harness (no dependencies)
  grid_geometry_test.cpp    Projection/unprojection checks
  tile_grid_test.cpp        Bounds, layering and walkability of the tile grid
  pathfinding_test.cpp      A* against a reference Dijkstra, plus fuzzing
  game_logic_test.cpp       Stats, combat resolution and actor behaviour
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

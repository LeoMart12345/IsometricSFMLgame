# IsometricSFMLgame — Architecture

How the game is put together: the folder layout, the frame loop, the state
stack, the isometric projection and the drawing order.

## Modules

| Folder | Responsibility |
|---|---|
| `src/core` | Game shell: window, frame loop, state stack, input polling, texture loading, asset path resolution, tunables (`constants.hpp`), assertions (`assert.hpp`) and the seeded PCG32 RNG (`rng.hpp`). |
| `src/game` | The SFML-free simulation: `stats` (attributes, derived values, experience curve), `combat` (hit chance, damage, resistances), `actor` (health, mana, levels, cooldown, movement) and `pathfinding` (A*). |
| `src/states` | `GameState` interface plus the two screens (`GameStateStart`, `GameStateEditor`). |
| `src/map` | `Map` (layered tiles, hover preview, draw order), `Tile`, `TileAtlas`, `TileType`, `TileGrid` (plain data grid for the simulation), `tile_properties.hpp` (move cost, walkability) and `grid_geometry.hpp` — the SFML-free projection math. |
| `src/ui` | Reusable widgets. Currently `Button`. |
| `src/animation` | Sprite-sheet helpers: `Animation` (frame range) and `AnimationHandler` (advances the visible frame). |
| `tests` | Headless test suites (see [Tests](#tests)); no SFML, no window. |

Headers are included relative to `src/` (for example `#include "map/tile.hpp"`),
so the include directory is `IsometricSFMLgame/src`.

## Object graph

```mermaid
classDiagram
    class Game {
        -RenderWindow window_
        -InputHandler input_
        -TextureManager& textures_
        -TileAtlas tileAtlas_
        -vector~unique_ptr~GameState~~ states_
        -unique_ptr~GameState~ pendingState_
        -bool pendingPop_
        +Game(TextureManager&)
        +pushState(unique_ptr~GameState~)
        +changeState(unique_ptr~GameState~)
        +popState()
        +gameLoop()
        +window() RenderWindow&
        +input() InputHandler&
        +textures() TextureManager&
        +tileAtlas() const TileAtlas&
    }

    class GameState {
        <<abstract>>
        #Game& game_
        +handleInput(const Event&)*
        +update(float dt)*
        +draw(float dt)*
    }

    class GameStateStart {
        -View view_
        -Sprite background_
        -startEditor()
    }

    class GameStateEditor {
        -Map map_
        -View gameView_
        -View guiView_
        -TileType currentTileType_
        -int currentZLevel_
        -handleKey(Key)
        -placeTileAtMouse(const MouseButtonPressed&)
        -updateHover()
        -selectTile(TileType)
    }

    class Map {
        -int width_
        -int height_
        -int layers_
        -vector~Tile~ tiles_
        -vector~size_t~ drawOrder_
        -const TileAtlas& tileAtlas_
        -Tile hoverTile_
        -bool hoverActive_
        +placeTile(x, y, z, type)
        +updateHover(Vector3i, TileType)
        +clearHover()
        +draw(RenderTarget&)
        +drawOutline(RenderTarget&, z)
        +drawHover(RenderTarget&)
        +gridToScreen(x, y, z) Vector2f
        +screenToGrid(Vector2f, z) Vector3i
        +getCenter() Vector2f
        +isInBounds(x, y, z) bool
        +isEmpty(x, y, z) bool
    }

    class Tile {
        -Sprite sprite_
        -TileType type_
        +setType(TileType)
        +setPosition(Vector2f)
        +setOpacity(uint8_t)
        +draw(RenderTarget&)
    }

    class TextureManager {
        -map~string, Texture~ textures_
        +loadTexture(name, path) bool
        +createPlaceholder(name, Color)
        +has(name) bool
        +getRef(name) const Texture&
    }

    class InputHandler {
        +update()
        +handleEvent(const Event&)
        +isMousePressed(Button) bool
        +isMouseJustPressed(Button) bool
        +isMouseReleased(Button) bool
        +isMouseDragging(Button, float) bool
        +isKeyPressed(Key) bool
        +isKeyJustPressed(Key) bool
        +isKeyReleased(Key) bool
        +getMouseScreenPos() Vector2i
        +getMouseWorldPos(const View&) Vector2f
    }

    Game o-- GameState : owns stack
    GameState <|-- GameStateStart
    GameState <|-- GameStateEditor
    GameStateEditor *-- Map
    Map o-- Tile
    Map --> TileAtlas : copies prototypes
    Game --> TextureManager : reference
    Game --> InputHandler
```

`Game` owns the resources (`TextureManager` is created by `main` and passed by
reference; `TileAtlas` holds one prototype `Tile` per type). States only hold
references, which is why the atlas and the texture manager outlive every state.
`Map` copies its initial tiles from the atlas, so tiles can change type without
touching the prototypes.

## Frame loop

The window is created from `config::windowWidth` × `config::windowHeight`
(1920×1080), clamped to the desktop size reported by
`sf::VideoMode::getDesktopMode()` so it still fits on smaller or scaled screens.
Every state sizes its views from `window.getSize()`, so nothing else depends on
the requested size.

```mermaid
sequenceDiagram
    participant Main
    participant Game
    participant Input as InputHandler
    participant State as GameState (top)
    participant Window

    Main->>Game: gameLoop()
    loop while window.isOpen()
        Game->>Game: dt = clock.restart()
        Game->>Input: update() (snapshot previous frame)
        loop while pollEvent() returns an event
            Game->>Input: handleEvent(event)
            alt Event::Closed
                Game->>Window: close()
            else any other event
                Game->>State: handleInput(event)
            end
        end
        Game->>State: update(dt)
        Game->>Window: clear(Color(30, 30, 40))
        Game->>State: draw(dt)
        Game->>Window: display()
        Game->>Game: applyPendingTransition()
    end
```

Events are polled in exactly one place (`Game::gameLoop`) and forwarded to the
top state, so a state never needs its own event loop and cannot miss or reorder
events.

## State transitions

Transitions are **deferred**: `changeState()` and `popState()` only record an
intent, and `Game::applyPendingTransition()` performs the swap after the frame
has finished drawing.

```mermaid
stateDiagram-v2
    [*] --> Start
    Start --> Editor : Enter / Space (changeState)
    Start --> [*] : Escape or window closed
    Editor --> Start : Escape (changeState)
    Editor --> [*] : window closed
```

Why deferred matters: a state requests its own replacement from inside
`handleInput()` (for example `Escape` in the editor). Destroying it immediately
would leave the loop calling methods on a freed object. With the deferred queue
the old state stays alive until the frame is over, and only the *top* state is
replaced (`pushState()` additionally keeps the previous state running below).

## Isometric projection

The math lives in `map/grid_geometry.hpp` as pure functions, with no SFML
dependency, so it can be unit tested without a window.

Convention: **grid coordinates address the centre of a cell**, `z` counts up.

```
halfW = tileWidth / 2 = 32        halfH = tileHeight / 2 = 16

project(x, y, z)   -> px = (x - y) * halfW
                      py = (x + y) * halfH - z * halfH

unproject(px, py, z) -> a = px / halfW
                        b = (py + z * halfH) / halfH
                        x = (a + b) / 2
                        y = (b - a) / 2

snap(x, y)          -> (lround(x), lround(y))
```

```mermaid
flowchart LR
    G["grid (x, y, z)<br/>float or int"] -->|"iso::project"| S["screen (px, py)"]
    S -->|"iso::unproject(px, py, z)"| F["grid (fractional)"]
    F -->|"iso::snap"| C["cell (int x, int y)"]
```

Because the projection is linear and invertible, every screen point inside a
tile's diamond unprojects to grid coordinates within ±0.5 of that tile's
centre, so rounding picks the same cell the user visibly clicked. `Map` wraps
these helpers in `gridToScreen()` / `screenToGrid()`.

`snap()` uses `std::lround`, not a truncating cast, which is what makes clicks
left of or above the origin (negative coordinates) resolve to the correct cell.

The grid outline uses the same projection on half-offset coordinates: the line
for column edge `x` runs from `project(x - 0.5, -0.5, z)` to
`project(x - 0.5, height - 0.5, z)`. That places the lines exactly on the
diamond corners of the outermost tiles (regression-tested in
`tests/grid_geometry_test.cpp`).

## Drawing

`Map` builds its painter's-algorithm order once in its constructor (tiles never
move): cells are sorted by `x + y` and, within the same diagonal, by `z`, so
nearer and higher tiles are drawn last.

```mermaid
flowchart TB
    A["Map::draw(): iterate drawOrder_"] --> B["tiles back to front"]
    B --> C["Map::drawOutline(currentZLevel)"]
    C --> D["Map::drawHover(): translucent preview"]
```

Two views are used by the editor:

* `gameView_` — same pixel size as the window, centred on the map centre and
  zoomed by `config::editorViewZoom`; used for all world-space drawing and for
  converting mouse pixels via `mapPixelToCoords(position, gameView_)`.
* `guiView_` — identity mapping in window pixels, installed *after* the world
  drawing; HUD elements (and the pixel-space `Button` hit tests) use it.

## Tile placement

```mermaid
sequenceDiagram
    participant User
    participant Editor as GameStateEditor
    participant Map
    participant Tile

    User->>Editor: mouse move (once per frame in update())
    Editor->>Map: screenToGrid(input.getMouseWorldPos(gameView), currentZLevel)
    Editor->>Map: updateHover(gridPos, currentTileType)
    Map->>Tile: copy prototype, opacity 128 (only if in bounds and empty)

    User->>Editor: left/right click (Event::MouseButtonPressed)
    Editor->>Map: screenToGrid(mapPixelToCoords(pos, gameView), currentZLevel)
    Editor->>Map: placeTile(x, y, z, type or VOID)
    Map->>Tile: setType + setPosition for the clicked cell
```

`TileType::VOID` is a command, never a stored state: `Map::placeTile` turns it
into `TileType::EMPTY` with the empty sprite. Out-of-bounds clicks and hover
positions are ignored by `Map`.

## Input

`InputHandler` keeps `current`/`previous` arrays for mouse buttons and keys, so
both level-triggered (`isKeyPressed`) and edge-triggered (`isKeyJustPressed`)
queries work. Two details matter:

* `sf::Keyboard::Key::Unknown` is `-1` and `sf::Mouse::Button` is a scoped enum;
  every enum-to-index conversion goes through `isValidKey()` /
  `isValidMouseButton()` so an unknown value cannot index out of bounds.
* Drag detection stores the exact press position from
  `Event::MouseButtonPressed`, so `isMouseDragging()` reports pixel-accurate
  distances even when the cursor is moved quickly.

## Assets

`core/asset_path.cpp` resolves art files by trying, in order:

1. the path relative to the working directory,
2. the executable's directory and up to four parent directories (covers
   `x64/Debug/`, `build/`, `build/Debug/`, ...),
3. the original relative path (so the error message shows what was expected).

`Game::loadTextures()` probes the filesystem before asking SFML to decode
anything (`std::filesystem::is_regular_file()`), because a failed
`sf::Texture::loadFromFile()` prints a three line error per file — with the art
files absent that turned a normal start-up into a wall of red text. Missing
files are collected and reported once, as a group, and each one is replaced by
`TextureManager::createPlaceholder()`, a solid-colour diamond, so the game is
playable with an empty `assets/` folder. A file that exists but cannot be
decoded is still reported by `TextureManager::loadTexture()`; that is a broken
asset rather than an expected state.

## Tests

The simulation layer is deliberately free of SFML so it can be tested headlessly;
each suite is a separate executable registered with CTest.

`tests/grid_geometry_test.cpp` includes `map/grid_geometry.hpp` only and checks:

* project → unproject → snap round-trips for every cell and layer,
* clicks inside a diamond select that cell, clicks outside do not,
* neighbour spacing and the z offset,
* the grid outline corners coincide with the outer diamond vertices.

`tests/tile_grid_test.cpp` covers the plain data grid the simulation runs on:
out-of-bounds reads are safe (writes are rejected, `at()`/`index()` assert),
`TileType::VOID` is never stored, and walkability/move cost come from
`tile_properties.hpp`.

`tests/pathfinding_test.cpp` compares A* against an independent, deliberately
naive Dijkstra over the same movement rules (8 directions, no corner cutting,
terrain cost), including a 60-round deterministic fuzz test over random maps.
It also pins down the `PathResult` contract: `reachedGoal` means the actor is
standing *on* the goal, `path` contains the cells to step through.

`tests/game_logic_test.cpp` covers `stats`, `combat` and `actor`: the experience
curve round-trips, hit chance stays inside 5-95% and never drops when attack
rating rises, armor/resistance only ever reduce damage, the same seed replays the
same fight, and a full monster-versus-player fight is played out headlessly.

Build and run with
`cmake -S . -B build -DISO_BUILD_GAME=OFF && cmake --build build && ctest --test-dir build --output-on-failure`.

Two rules the tests rely on and that new simulation code should keep:

* **Determinism.** `iso::Rng` is the only source of randomness. `resolveAttack()`
  always consumes exactly three rolls (hit, damage, critical) so a seed replays a
  session no matter which attacks land.
* **Assert relationships, not magic numbers.** A test saying "more vitality means
  more health" survives balance tuning; a test asserting `maxHealth == 137` does
  not.

## Building and smoke-testing the graphical binary

`scripts/build-linux.sh` prepares everything the CMake build needs on Linux
without root: it extracts the missing X11/GL/UDev `-devel` RPMs into a private
sysroot (`dnf` + `rpm2cpio`), builds SFML 3 shared into a private prefix, then
configures, builds and runs `ctest` for this project. Re-running it is cheap
because each stage is skipped when it is already done.

The graphical executable is verified by running it against a virtual X server
and grabbing the framebuffer (`XGetImage`), which exercises the whole chain —
window creation, GL context, texture atlas with generated placeholders,
projection, drawing order, and event handling:

```bash
Xvfb :99 -screen 0 1920x1080x24 &
DISPLAY=:99 ./build/IsometricGame      # title screen; Enter opens the editor
```

Known-harmless messages in such a session: `TextEntered event won't be able to
return unicode` (no input method) and `Setting vertical sync not supported`
(software GL). With an empty `assets/` folder the game prints one `[assets] ...`
block listing the files it replaced with generated placeholders; that is
expected and not an error.

## Not wired up yet

`Button` and `AnimationHandler` are complete, self-contained components with no
dependencies on the rest of the game, but no state instantiates them yet. To use
them:

* add a `Button` member to a state, forward `Event::MouseButtonPressed` /
  `MouseButtonReleased` to `Button::handleEvent()`, set a callback with
  `setOnClick()`, and draw it after `window.setView(guiView_)`;
* give a `Tile` an `AnimationHandler`, call `update(dt)` each frame and pass
  `getBounds()` to the sprite's `setTextureRect()`.

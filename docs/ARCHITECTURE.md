# Architecture

The project is deliberately small: an SFML 3 window, a state stack with a title
screen and a map editor, a layered isometric tile map, and one animated player.
There is no simulation layer (no combat, pathfinding or tile grid) yet.

## Directory map

| Path | Responsibility |
|---|---|
| `src/main.cpp` | Resolves the asset root, checks for a display server, starts `Game`. |
| `src/core/game.*` | Owns the window, the textures, the tile atlas and the state stack; runs the frame loop. |
| `src/core/texture_manager.*` | Owns every `sf::Texture` and generates stand-ins for missing art. |
| `src/core/asset_path.*` | Finds `assets/` relative to the working directory or the executable (and up to four parents). |
| `src/core/constants.hpp` | Window, tile, map, camera and player tunables. |
| `src/map/grid_geometry.hpp` | Pure isometric projection/unprojection math, no SFML dependency. |
| `src/map/map.*` | Layered tile storage, painter's-order drawing, hover preview, grid outline. |
| `src/map/tile.*` | One tile: a sprite plus its `TileType`, scaled to `config::tileWidth`. |
| `src/map/tile_atlas.hpp` | `std::map<TileType, Tile>` of prototype tiles that `Map` copies from. |
| `src/map/tile_type.hpp` | The `TileType` enum. |
| `src/game/player.*` | The player sprite and its idle animation. |
| `src/states/game_state.hpp` | Abstract state: `handleInput`, `update`, `draw`. |
| `src/states/game_state_start.*` | Title screen (Enter/Space opens the editor, Escape quits). |
| `src/states/game_state_editor.*` | Tile placement, layer switching and hover preview. |
| `tests/grid_geometry_test.cpp` | Headless checks for the projection math. |

Include paths are rooted at `src/` (`#include "map/map.hpp"`).

## Frame loop

`Game::gameLoop()` (in `core/game.cpp`):

1. `dt = min(clock.restart(), config::maxFrameTime)` — a stalled frame (window
   drag, breakpoint, alt-tab) never makes the animation jump.
2. Poll every event, close the window on `Event::Closed`, and forward the rest to
   the top state's `handleInput()`.
3. Call `update(dt)`, clear the window, call `draw(dt)`, `display()`.
4. Apply a deferred state transition.

State changes are deferred (`pendingState_` / `pendingPop_`) so a state can
replace itself from inside its own callback without being destroyed while its
member function is still running. If the stack becomes empty and nothing is
pending, the loop closes the window instead of spinning.

## Resources

`main()` creates `TextureManager` before `Game`, so textures outlive every sprite
that references them. `Game` loads the textures, builds the tile atlas once, and
exposes `window()`, `textures()` and `tileAtlas()`. States are owned by `Game`
and only borrow those resources.

Missing art is a normal state, not an error. `Game::loadTextures()` probes the
filesystem first (so SFML does not print an error per file), and on failure asks
`TextureManager` for a generated stand-in: a 2:1 diamond for tiles, an animated
four-frame strip for the player. The names of missing files are reported once as
a group.

## Coordinates and the map

Cells are addressed by their **centre**. With `halfW = tileWidth / 2 = 32` and
`halfH = tileHeight / 2 = 16`:

```
project(x, y, z)     -> ( (x - y) * halfW, (x + y) * halfH - z * halfH )
unproject(screen, z) -> the fractional grid position
snap(grid)           -> the cell whose centre is closest
```

`Map::screenToGrid()` uses the inverse projection and rounds to the nearest cell,
so a click anywhere inside a diamond selects that cell. The map is
`config::mapWidth` x `config::mapHeight` x `config::mapLayers` (10 x 10 x 3) and
stores tiles in a flat `std::vector`, indexed as `(z * height + y) * width + x`.

Drawing order is precomputed once in the constructor: every tile index is sorted
by `x + y + z`, the standard isometric painter's order. Empty cells are skipped.
`drawOutline()` draws the grid lines along cell borders (half a cell away from
the centres) for the active z level, and `drawHover()` draws a translucent
prototype only over empty, in-bounds cells.

## Tiles

`Tile` holds an `sf::Sprite` and a `TileType`. Its origin is the centre of the
texture and it is scaled so its width is exactly `config::tileWidth`, which
keeps the footprint consistent for any source resolution. `Map` copies prototype
tiles out of the `TileAtlas` when a cell is filled, so tiles never share mutable
state. `TileType::VOID` is a command, not stored: placing it writes
`TileType::EMPTY` and restores the transparent empty sprite.

## Player and idle animation

`Player` is created by `GameStateEditor` on the centre cell and drawn after the
map. Its sheet (`assets/player.png`) is one horizontal strip of square frames:

* frame size = sheet height, frame count = sheet width / sheet height;
* origin = bottom centre, so the feet land on the tile centre returned by
  `Map::gridToScreen()`;
* the sprite is scaled so one frame is `config::playerScreenHeight` (64 px) tall;
* `update(dt)` accumulates time and advances the frame every
  `config::playerIdleFrameSeconds` (0.12 s), wrapping to frame 0.

This is the layout of a Diablo II animation conversion cropped to one idle
direction, so the real art can be dropped in unchanged. With no art file, the
generated four-frame figure bobs up and down, which keeps the animation visible
and testable.

The player is currently decorative (no movement) and is drawn on top of all
tiles; proper per-tile occlusion would be the next step if he starts moving
behind buildings.

## Editor

`GameStateEditor` owns the `Map` and the `Player`. It keeps a world view (centred
on the map, zoomed by `config::editorViewZoom`) and a pixel-perfect GUI view for
future HUD elements. Input:

* `Escape` returns to the title screen;
* `G` / `H` lower / raise the active z level (clamped to the map's layer count);
* `0` selects clear, `1`-`7` select a tile type;
* left click places the selection, right click clears;
* every update, the mouse position is unprojected to refresh the hover preview.

## Tests

`tests/grid_geometry_test.cpp` only includes `map/grid_geometry.hpp`, which has
no SFML dependency, so it builds and runs without a window:

```bash
cmake -S . -B build -DISO_BUILD_GAME=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Known limitations

* Maps are not saved or loaded.
* The player never moves and is not depth-sorted against the tiles.
* The map size, zoom and animation timing are compile-time constants.

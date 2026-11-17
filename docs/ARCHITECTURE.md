# Architecture

The project is deliberately small: an SFML 3 window, a layered isometric
`Map`, and the pure projection math it uses. There is no simulation, art
pipeline or state stack — the program opens straight into the tile editor.

## Directory map

| Path | Responsibility |
|---|---|
| `src/main.cpp` | Checks for a display server and starts `Game`. |
| `src/core/game.*` | Owns the window, map and view; handles input and runs the frame loop. |
| `src/core/constants.hpp` | Window, tile, map, camera and hover tunables. |
| `src/map/map.*` | Layered tile storage, placement, painter's-order drawing, hover preview and grid outline. |
| `src/map/grid_geometry.hpp` | Pure isometric projection/unprojection math, no SFML dependency. |
| `src/map/tile_type.hpp` | The tile kinds the editor can place. |

## Ownership

`Game` owns `sf::RenderWindow`, `Map` and `sf::View` by value. `Map` owns a flat
`std::vector<TileType>` plus a parallel `std::vector<sf::Vector2f>` holding the
centre of every cell. Nothing is loaded from disk, so construction only fails
on invalid map dimensions.

## Coordinates and the map

Cells are addressed by their **centre**. With `halfW = tileWidth / 2 = 32` and
`halfH = tileHeight / 2 = 16`:

```
project(x, y, z)     -> ( (x - y) * halfW, (x + y) * halfH - z * halfH )
unproject(screen, z) -> the fractional grid position
snap(grid)           -> the cell whose centre is closest
```

`Map::screenToGrid()` uses the inverse projection and rounds to the nearest
cell, so a click anywhere inside a diamond selects that cell. The map is
`config::mapWidth` x `config::mapHeight` x `config::mapLayers` (10 x 10 x 3) and
stores cell types in a flat `std::vector`, indexed as
`(z * height + y) * width + x`.

Drawing order is precomputed once in the constructor: every cell index is
sorted by `x + y + z`, the standard isometric painter's order. A stable sort
keeps equally deep cells in index order.

## Drawing

Each non-empty cell is an `sf::ConvexShape` diamond of `config::tileWidth` x
`config::tileHeight`, centred on the precomputed cell position. `Map::draw()`
creates the diamond once, walks the painter's order, and sets its position and
fill colour per cell; `colorOf()` maps a `TileType` to that colour.

`drawHover()` draws the same diamond with `config::hoverAlpha` only over an
empty, in-bounds cell, and `drawOutline()` draws the grid lines of the active z
level. The grid borders sit half a cell away from the cell centres.

`TileType::VOID` is a command rather than a stored state: `placeTile()` writes
`TileType::EMPTY` for it.

## Editor

`Game` keeps the selected tile type (grass by default) and the active z level
(0 by default). Input:

* `Escape` quits;
* `G` / `H` lower / raise the active z level (clamped to the layer count);
* `0` selects the eraser, `1`-`7` select a tile type through `kTileKeys`;
* left click places the selection, right click clears the cell.

The mouse position is converted with the world view (`mapPixelToCoords`) and
unprojected on the active z level, so the hover preview and the click land on
the same cell. The window is authored at 1920x1080, shrunk if the desktop is
smaller, and the view is centred on the map and zoomed by
`config::editorViewZoom`.

## Frame

```
poll events -> handle close / keys / clicks
            -> refresh hover from the mouse position
            -> clear
            -> draw tiles, hover, outline
            -> display
```

There is no `update(dt)` step because nothing animates or simulates.

## Tests

`tests/grid_geometry_test.cpp` only includes `map/grid_geometry.hpp`, which has
no SFML dependency, so it builds and runs without a window:

```bash
cmake -S . -B build -DISO_BUILD_GAME=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

The test checks projection round-trips, clicks inside a diamond, neighbour
spacing, the z shift and the grid outline corners.

## Known limitations

* Maps are not saved or loaded.
* Tiles are flat coloured diamonds; there is no texture or sprite pipeline.
* Map size, zoom and tile colours are compile-time constants.

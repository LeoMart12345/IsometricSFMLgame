// tile_grid.hpp
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "map/tile_properties.hpp"
#include "map/tile_type.hpp"

// Storage-only layered grid of tile types.
//
// This is the authority on *what* is on the map; Map (rendering) and the game
// simulation (pathfinding, AI) both read from it. It has no SFML dependency so
// every rule below can be unit tested without a window or a GPU.
//
// Cells are addressed as (x, y, z) with x in [0, width), y in [0, height) and
// z in [0, layers). Out-of-bounds *queries* are safe and answer "nothing here";
// out-of-bounds *writes* are rejected (return false) and out-of-bounds `at()`
// calls are a programming error and assert.
class TileGrid
{
public:
    // Throws std::invalid_argument when a dimension is not positive.
    explicit TileGrid(int width, int height, int layers, TileType fill = TileType::EMPTY);

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }
    [[nodiscard]] int layers() const { return layers_; }

    [[nodiscard]] std::size_t cellCount() const;

    [[nodiscard]] bool isInBounds(int x, int y, int z) const;

    // Flat index of a cell. Asserts that the position is inside the grid.
    [[nodiscard]] std::size_t index(int x, int y, int z) const;

    // Type stored at the position. Asserts that the position is inside the grid.
    [[nodiscard]] TileType at(int x, int y, int z) const;

    // Stores `type`. Returns false when the position is out of bounds (nothing
    // is written) or when the cell already had that type. TileType::VOID is
    // translated to TileType::EMPTY: VOID is a command, never a stored value.
    bool set(int x, int y, int z, TileType type);

    // Stores `type` without the equality short cut. Used by loaders/generators
    // that may write the same value twice.
    void store(int x, int y, int z, TileType type);

    void fill(TileType type);

    // True when the cell exists and holds TileType::EMPTY. Out-of-bounds
    // positions are *not* empty (there is no cell there at all).
    [[nodiscard]] bool isEmpty(int x, int y, int z) const;

    // Walkability/move cost of a cell; false/0 outside the grid.
    [[nodiscard]] bool isWalkable(int x, int y, int z) const;
    [[nodiscard]] int moveCost(int x, int y, int z) const;

    [[nodiscard]] const std::vector<TileType>& cells() const { return cells_; }

    // Checks the invariants this class promises: matching size, no VOID stored,
    // and a consistent move cost table. Returns false and fills `reason` when
    // something is off.
    [[nodiscard]] bool validate(std::string* reason = nullptr) const;

private:
    int width_;
    int height_;
    int layers_;
    std::vector<TileType> cells_;
};

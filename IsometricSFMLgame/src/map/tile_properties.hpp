// tile_properties.hpp
#pragma once

#include "map/tile_type.hpp"

// Gameplay meaning of a tile type. Deliberately separate from rendering: the
// simulation (pathfinding, monster AI) must be testable without a window, and
// the same rules have to apply to the editor and to play mode.
struct TileProperties
{
    bool walkable = false;

    // Reservable for line of sight / field of view. Trees and buildings block
    // sight, open ground does not.
    bool blocksSight = false;

    // Pathfinding cost of entering the cell (1 = normal). Always >= 1 when the
    // cell is walkable, 0 when it is not, so `moveCost > 0` is a valid
    // walkability test.
    int moveCost = 0;
};

// Rules per tile type. This switch intentionally has no default branch: adding
// an enumerator without describing it here is a compile error with -Wswitch,
// which is exactly the reminder we want.
[[nodiscard]] constexpr TileProperties tileProperties(TileType type)
{
    switch (type)
    {
    // No cell at all, and the "clear this cell" command. Never stored in a grid.
    case TileType::VOID:
        return {false, false, 0};

    // Open ground and dungeon floors.
    case TileType::GRASS:
        return {true, false, 1};

    // Woods slow you down and hide what is behind them.
    case TileType::FOREST:
        return {true, true, 2};

    // Water is impassable for walkers (monsters and the player alike).
    case TileType::WATER:
        return {false, false, 0};

    // Buildings: their ground floor can be walked on (interiors/doorways) but
    // they block sight, which makes corridors matter.
    case TileType::RESIDENTIAL:
    case TileType::COMMERCIAL:
    case TileType::INDUSTRIAL:
        return {true, true, 1};

    // Roads are the fast way through a level.
    case TileType::ROAD:
        return {true, false, 1};

    // "Empty" means there is no floor here (the default of a fresh grid), so it
    // is neither walkable nor a valid pathfinding node.
    case TileType::EMPTY:
        return {false, false, 0};
    }

    return {false, false, 0};
}

[[nodiscard]] constexpr bool isWalkable(TileType type)
{
    return tileProperties(type).walkable;
}

[[nodiscard]] constexpr int moveCost(TileType type)
{
    return tileProperties(type).moveCost;
}

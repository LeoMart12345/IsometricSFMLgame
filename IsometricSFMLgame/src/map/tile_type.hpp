// tile_type.hpp
#pragma once

// Every kind of cell the editor can place. VOID is a command (clear the cell)
// rather than something that is ever stored on the map; EMPTY is the stored
// "nothing here" state.
enum class TileType
{
    VOID,
    GRASS,
    FOREST,
    WATER,
    RESIDENTIAL,
    COMMERCIAL,
    INDUSTRIAL,
    ROAD,
    EMPTY
};


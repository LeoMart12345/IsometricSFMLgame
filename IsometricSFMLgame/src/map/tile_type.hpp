// tile_type.hpp
#pragma once

#include <string>

// Every kind of cell the editor can place. VOID is a command (clear the cell)
// rather than something that is ever stored on the map.
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

// Human readable name, used for debugging/UI labels.
[[nodiscard]] std::string tileTypeToStr(TileType type);

// Lowercase texture-atlas key. VOID maps to "empty" because there is no "void"
// texture: clearing a tile simply restores the empty sprite.
[[nodiscard]] std::string tileTypeToTextureName(TileType type);

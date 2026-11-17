// tile_type.hpp
#pragma once

#include <array>
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

// Canonical list of every enumerator, in declaration order.
//
// New tile types are added here and in tileProperties(); the switch in
// tileTypeToStr() has no default branch, so the compiler also flags a type that
// was forgotten there. Game uses this list to verify that the tile atlas really
// provides a sprite for every type before the first map is built.
inline constexpr std::array<TileType, 9> allTileTypes{TileType::VOID,    TileType::GRASS,
                                                      TileType::FOREST,  TileType::WATER,
                                                      TileType::RESIDENTIAL, TileType::COMMERCIAL,
                                                      TileType::INDUSTRIAL,  TileType::ROAD,
                                                      TileType::EMPTY};

// True only for a real enumerator. A value produced by static_cast<TileType>(99)
// is not a tile type and must not silently look like a valid, empty cell.
[[nodiscard]] constexpr bool isValidTileType(TileType type)
{
    for (const TileType candidate : allTileTypes)
    {
        if (candidate == type)
            return true;
    }

    return false;
}

// False for VOID, which is only ever a request to clear a cell.
[[nodiscard]] constexpr bool isPlaceable(TileType type)
{
    return type != TileType::VOID;
}

// Human readable name, used for debugging/UI labels.
[[nodiscard]] std::string tileTypeToStr(TileType type);

// Lowercase texture-atlas key. VOID maps to "empty" because there is no "void"
// texture: clearing a tile simply restores the empty sprite.
[[nodiscard]] std::string tileTypeToTextureName(TileType type);

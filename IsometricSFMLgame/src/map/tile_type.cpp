// tile_type.cpp
#include "map/tile_type.hpp"

#include <algorithm>
#include <cctype>

std::string tileTypeToStr(TileType type)
{
    switch (type)
    {
    case TileType::VOID:        return "Void";
    case TileType::GRASS:       return "Grass";
    case TileType::FOREST:      return "Forest";
    case TileType::WATER:       return "Water";
    case TileType::RESIDENTIAL: return "Residential";
    case TileType::COMMERCIAL:  return "Commercial";
    case TileType::INDUSTRIAL:  return "Industrial";
    case TileType::ROAD:        return "Road";
    case TileType::EMPTY:       return "Empty";
    }

    return "Unknown";
}

std::string tileTypeToTextureName(TileType type)
{
    std::string name = tileTypeToStr(type);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });

    // VOID has no texture of its own; it resets a cell to the empty sprite.
    if (name == "void")
        name = "empty";

    return name;
}

#pragma once

#include <SFML/Graphics.hpp>
#include <string>

enum class TileType { VOID, GRASS, FOREST, WATER, RESIDENTIAL, COMMERCIAL, INDUSTRIAL, ROAD, EMPTY};

std::string tileTypeToStr(TileType type);

// Returns the lowercase texture-atlas key for a tile type.
// VOID maps to "empty" (there is no "void" texture).
std::string tileTypeToTextureName(TileType type);

class Tile
{
public:
    sf::Sprite sprite;
    TileType tileType;

    // Delete default constructor
    Tile() = delete;

    // Simple constructor
    Tile(unsigned int tileSize,
        unsigned int height,
        sf::Texture& texture,
        TileType tileType);

    void draw(sf::RenderWindow& window, float dt);
    void update();
};

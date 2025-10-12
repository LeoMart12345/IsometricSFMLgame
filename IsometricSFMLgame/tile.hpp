#pragma once
#include <SFML/Graphics.hpp>

enum class TileType { VOID, GRASS, FOREST, WATER, RESIDENTIAL, COMMERCIAL, INDUSTRIAL, ROAD, EMPTY};

std::string tileTypeToStr(TileType type);

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
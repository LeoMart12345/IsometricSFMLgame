#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory> // For smart pointers
#include "Tile.hpp"
#include "texture_manager.hpp"

class Map {
private:
    std::vector<std::vector<std::vector<std::shared_ptr<Tile>>>> tiles;
    float tileWidth, tileHeight;
    TextureManager& texmgr;  // Add reference to texture manager


    //for tile HOVER 
    std::shared_ptr<Tile> hoverTile;
    bool hoverActive = false;
    sf::Vector3i hoverPos;

public:
    Map(int width, int height, int layers, TextureManager& textureManager);
    sf::Vector2f getScreenPos(int x, int y, int z);
    sf::Vector3i screenToGrid(float screenX, float screenY, int currentZ);
    void placeTile(int x, int y, int z, TileType type);
    void draw(sf::RenderWindow& window);
    float getTileWidth() const { return tileWidth; }
    float getTileHeight() const { return tileHeight; }

    //For map stuff can make a seperate class for this probably
    void drawOutline(sf::RenderWindow& window);
    bool isInBounds(int x, int y, int z) const;
    bool isEmpty(int x, int y, int z) const;

    void updateHover(const sf::Vector3i& position, TileType type);
    void drawHover(sf::RenderWindow& window) const;
    void clearHover();

};
#include "Map.hpp"
#include <SFML/Graphics.hpp>
#include "texture_manager.hpp"
#include "tile.hpp" 
#include "game.hpp"
#include "game_state.hpp"
#include <iostream>

Map::Map(int width, int height, int layers, TextureManager& textureManager) :
    tileWidth(64.f), tileHeight(32.f), texmgr(textureManager)
{
    std::cout << "FORCE-INITIALIZING with " << layers << " layers\n";

    hoverTile = std::make_shared<Tile>(8, 1, texmgr.getRef("empty"), TileType::EMPTY);

    // Clear and properly resize
    tiles.clear();
    tiles.resize(width);

    for (auto& col : tiles) {
        col.resize(height);
        for (auto& layer : col) {
            layer.resize(layers);

            // Initialize each tile
            for (auto& tile : layer) {
                tile = std::make_shared<Tile>(
                    8, 1, texmgr.getRef("empty"), TileType::EMPTY
                );
            }
        }
    }

    // Verification
    std::cout << "CONFIRMED dimensions: "
        << tiles.size() << "x"
        << tiles[0].size() << "x"
        << tiles[0][0].size() << "\n";
}

void Map::draw(sf::RenderWindow& window) {
    for (size_t z = 0; z < tiles[0][0].size(); ++z) {
        for (size_t y = 0; y < tiles[0].size(); ++y) {
            for (size_t x = 0; x < tiles.size(); ++x) {
                if (tiles[x][y][z]->tileType != TileType::EMPTY) {
                    tiles[x][y][z]->sprite.setPosition(getScreenPos(x, y, z));
                    window.draw(tiles[x][y][z]->sprite);
                }
            }
        }
    }
}

bool Map::isInBounds(int x, int y, int z) const {
    return x >= 0 && x < static_cast<int>(tiles.size()) &&
        y >= 0 && y < static_cast<int>(tiles[0].size()) &&
        z >= 0 && z < static_cast<int>(tiles[0][0].size());
}

sf::Vector2f Map::getScreenPos(int x, int y, int z) {
    return {
        (x - y) * tileWidth / 2,
        (x + y) * tileHeight / 2 - z * (tileHeight / 2)
    };
}

sf::Vector3i Map::screenToGrid(float screenX, float screenY, int currentZ) {
    float gridX = (screenX / tileWidth + screenY / tileHeight);
    float gridY = (screenY / tileHeight - screenX / tileWidth);
    return {
        static_cast<int>(std::floor(gridX)),
        static_cast<int>(std::floor(gridY)),
        currentZ
    };
}



void Map::placeTile(int x, int y, int z, TileType type) {
    // Debug: Print attempt information
    std::cout << "\nAttempting to place tile at (" << x << "," << y << "," << z << "):\n";
    std::cout << "Map bounds - X:" << tiles.size() - 1
        << " Y:" << tiles[0].size() - 1
        << " Z:" << tiles[0][0].size() - 1 << "\n";

    if (isInBounds(x, y, z))
    {
        std::string texName = tileTypeToStr(type);
        std::transform(texName.begin(), texName.end(), texName.begin(), ::tolower);

        // Debug: Texture verification
        std::cout << "Texture name: " << texName << "\n";
        if (!texmgr.getRef(texName).getNativeHandle()) {
            std::cerr << "ERROR: Texture not loaded for '" << texName << "'\n";
            return;
        }

        tiles[x][y][z]->tileType = type;
        tiles[x][y][z]->sprite.setTexture(texmgr.getRef(texName));

        // Debug: Position verification
        sf::Vector2f screenPos = getScreenPos(x, y, z);
        std::cout << "Successfully placed at screen position ("
            << screenPos.x << "," << screenPos.y << ")\n";
        std::cout << "Texture size: "
            << texmgr.getRef(texName).getSize().x << "x"
            << texmgr.getRef(texName).getSize().y << "\n";
    }
    else {
        // error reporting
        std::cerr << "FAILED: Out of bounds - ";
        if (x < 0 || x >= static_cast<int>(tiles.size()))
            std::cerr << "X=" << x << " invalid (0-" << tiles.size() - 1 << ") ";
        if (y < 0 || y >= static_cast<int>(tiles[0].size()))
            std::cerr << "Y=" << y << " invalid (0-" << tiles[0].size() - 1 << ") ";
        if (z < 0 || z >= static_cast<int>(tiles[0][0].size()))
            std::cerr << "Z=" << z << " invalid (0-" << tiles[0][0].size() - 1 << ") ";
        std::cerr << "\n";
    }
}

void Map::drawOutline(sf::RenderWindow& window) {
    // We'll use a vertex array since we have many lines
    sf::VertexArray grid(sf::PrimitiveType::Lines);

    // Draw vertical lines (along x-axis)
    for (size_t x = 0; x <= tiles.size(); ++x) {
        sf::Vector2f top = getScreenPos(x, 0, 0);
        sf::Vector2f bottom = getScreenPos(x, tiles[0].size() - 1, 0);

        grid.append(sf::Vertex(top, sf::Color(255, 255, 255, 64)));
        grid.append(sf::Vertex(bottom, sf::Color(255, 255, 255, 64)));
    }

    // Draw horizontal lines (along y-axis)
    for (size_t y = 0; y <= tiles[0].size(); ++y) {
        sf::Vector2f left = getScreenPos(0, y, 0);
        sf::Vector2f right = getScreenPos(tiles.size() - 1, y, 0);

        grid.append(sf::Vertex(left, sf::Color(255, 255, 255, 64)));
        grid.append(sf::Vertex(right, sf::Color(255, 255, 255, 64)));
    }

    window.draw(grid);
}

//returns true if its out of bounds or if the tile type is empty
bool Map::isEmpty(int x, int y, int z) const {
    if (!isInBounds(x, y, z)) return true;
    return tiles[x][y][z]->tileType == TileType::EMPTY;
}

void Map::drawHover(sf::RenderWindow& window) const {
    if (hoverActive) {
        window.draw(hoverTile->sprite);
    }
}

void Map::clearHover() {
    hoverActive = false;
}

void Map::updateHover(const sf::Vector3i& position, TileType type) {
    if (isInBounds(position.x, position.y, position.z) &&
        isEmpty(position.x, position.y, position.z)) {
        hoverPos = position;
        hoverActive = true;

        std::string texName = tileTypeToStr(type);
        hoverTile = std::make_shared<Tile>(
            8, 1, texmgr.getRef("grass"), TileType::GRASS  // Force grass texture
        );

        hoverTile->sprite.setColor(sf::Color(255, 255, 255, 128)); // 50% opacity
        hoverTile->sprite.setPosition(getScreenPos(position.x, position.y, position.z));
    }
    else {
        hoverActive = false;
    }
}
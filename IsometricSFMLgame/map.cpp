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

}

void Map::draw(sf::RenderWindow& window) {
    if (tiles.empty() || tiles[0].empty() || tiles[0][0].empty()) return;

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
    if (tiles.empty() || tiles[0].empty() || tiles[0][0].empty()) return false;
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
    // Inverse of getScreenPos:
    //   sx = (x - y) * tileWidth  / 2
    //   sy = (x + y) * tileHeight / 2 - z * tileHeight / 2
    // Solve for x, y:
    //   x = sx / tileWidth  + sy / tileHeight
    //   y = sy / tileHeight - sx / tileWidth
    const float halfW = tileWidth  / 2.f;
    const float halfH = tileHeight / 2.f;

    const float adjustedY = screenY + currentZ * halfH;

    const float gridX = (screenX / halfW + adjustedY / halfH) / 2.f;
    const float gridY = (adjustedY / halfH - screenX / halfW) / 2.f;

    return {
        static_cast<int>(std::floor(gridX)),
        static_cast<int>(std::floor(gridY)),
        currentZ
    };
}



void Map::placeTile(int x, int y, int z, TileType type) {
    if (!isInBounds(x, y, z)) {
        return;
    }

    std::string texName = tileTypeToStr(type);
    std::transform(texName.begin(), texName.end(), texName.begin(), ::tolower);

    tiles[x][y][z]->tileType = type;
    tiles[x][y][z]->sprite.setTexture(texmgr.getRef(texName));
}

void Map::drawOutline(sf::RenderWindow& window) {
    if (tiles.empty() || tiles[0].empty()) return;

    const int width  = static_cast<int>(tiles.size());
    const int height = static_cast<int>(tiles[0].size());
    const sf::Color lineColor(255, 255, 255, 64);

    // We'll use a vertex array since we have many lines
    sf::VertexArray grid(sf::PrimitiveType::Lines);

    // Draw lines along the x-axis (one per y edge, inclusive)
    for (int x = 0; x <= width; ++x) {
        grid.append(sf::Vertex(getScreenPos(x, 0, 0), lineColor));
        grid.append(sf::Vertex(getScreenPos(x, height, 0), lineColor));
    }

    // Draw lines along the y-axis (one per x edge, inclusive)
    for (int y = 0; y <= height; ++y) {
        grid.append(sf::Vertex(getScreenPos(0, y, 0), lineColor));
        grid.append(sf::Vertex(getScreenPos(width, y, 0), lineColor));
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
        std::transform(texName.begin(), texName.end(), texName.begin(), ::tolower);

        hoverTile = std::make_shared<Tile>(
            8, 1, texmgr.getRef(texName), type
        );

        hoverTile->sprite.setColor(sf::Color(255, 255, 255, 128)); // 50% opacity
        hoverTile->sprite.setPosition(getScreenPos(position.x, position.y, position.z));
    }
    else {
        hoverActive = false;
    }
}

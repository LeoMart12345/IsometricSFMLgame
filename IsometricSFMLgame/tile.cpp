#include "tile.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>

std::string tileTypeToStr(TileType type)
{
    switch (type) {
    case TileType::VOID: return "Void";
    case TileType::GRASS: return "Grass";
    case TileType::FOREST: return "Forest";
    case TileType::WATER: return "Water";
    case TileType::RESIDENTIAL: return "Residential";
    case TileType::COMMERCIAL: return "Commercial";
    case TileType::INDUSTRIAL: return "Industrial";
    case TileType::ROAD: return "Road";
	case TileType::EMPTY: return "Empty";
                                                         
    default: return "Unknown";
    }
}

std::string tileTypeToTextureName(TileType type)
{
    // VOID has no texture of its own; it resets a tile to the empty sprite.
    if (type == TileType::VOID) return "empty";

    std::string name = tileTypeToStr(type);
    std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

Tile::Tile(unsigned int tileSize, unsigned int height, sf::Texture& texture, TileType type) :
    sprite(texture),
    tileType(type)
{
    // Use the full texture as the sprite's source rect.
    const sf::Vector2u texSize = texture.getSize();
    sprite.setTextureRect(sf::IntRect(
        sf::Vector2i(0, 0),
        sf::Vector2i(static_cast<int>(texSize.x), static_cast<int>(texSize.y))
    ));

    // Origin at the horizontal center and vertical center of the tile.
    // (height is reserved for future multi-tile sprites.)
    (void)height;
    sprite.setOrigin({ static_cast<float>(tileSize) / 2.f,
                       static_cast<float>(tileSize) / 2.f });
}

void Tile::draw(sf::RenderWindow& window, float /*dt*/)
{
    window.draw(sprite);
}

void Tile::update()
{
    // Tile update logic if needed
}

#include "Tile.hpp"
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

Tile::Tile(unsigned int tileSize, unsigned int height, sf::Texture& texture, TileType type) :
    sprite(texture),
    tileType(type)
{
    // Set texture rectangle to use entire 64x64 area
    sprite.setTextureRect(sf::IntRect(
        sf::Vector2i(0, 0),       // Top-left corner
        sf::Vector2i(64, 64)      // Size (must match texture) it doesnt right now NEED TO FIX!
    ));

    // Verify texture size matches expected dimensions ADD THIS LINE BACK IN WHEN TEXTURE IS ACTUALLY 64 x 64
    //assert(texture.getSize().x == 64 && texture.getSize().y == 64); 

    // Maintain isometric positioning
    //sprite.setOrigin({0.f, static_cast<float>(tileSize * (height - 1)) });
    sprite.setOrigin({32.f, 32.f }); // Half width, full height

}

void Tile::draw(sf::RenderWindow& window, float dt)
{
    window.draw(sprite);
}

void Tile::update()
{
    // Tile update logic if needed
}
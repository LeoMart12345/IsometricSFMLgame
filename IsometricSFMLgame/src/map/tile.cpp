// tile.cpp
#include "map/tile.hpp"

#include "core/constants.hpp"

Tile::Tile(const sf::Texture& texture, TileType type)
    : sprite_(texture), type_(type)
{
    const sf::Vector2u textureSize = texture.getSize();
    const float width = static_cast<float>(textureSize.x);
    const float height = static_cast<float>(textureSize.y);

    // Centre the sprite on its texture so Map can place tiles by their centre.
    sprite_.setOrigin({width / 2.f, height / 2.f});

    // Scale the art so one tile is exactly config::tileWidth pixels wide.
    if (width > 0.f)
    {
        const float scale = static_cast<float>(config::tileWidth) / width;
        sprite_.setScale({scale, scale});
    }
}

void Tile::setType(TileType type)
{
    type_ = type;
}

TileType Tile::getType() const
{
    return type_;
}

void Tile::setPosition(sf::Vector2f position)
{
    sprite_.setPosition(position);
}

void Tile::setOpacity(std::uint8_t alpha)
{
    sf::Color color = sprite_.getColor();
    color.a = alpha;
    sprite_.setColor(color);
}

void Tile::draw(sf::RenderTarget& target) const
{
    target.draw(sprite_);
}

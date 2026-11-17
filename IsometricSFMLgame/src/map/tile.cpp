// tile.cpp
#include "map/tile.hpp"

#include "core/constants.hpp"

Tile::Tile(const sf::Texture& texture, TileType type)
    : sprite_(texture), type_(type)
{
    const sf::Vector2f size = static_cast<sf::Vector2f>(texture.getSize());

    // Centre the sprite on its texture so Map can place tiles by their centre.
    sprite_.setOrigin(size / 2.f);

    // Scale the art so one tile is exactly config::tileWidth pixels wide.
    if (size.x > 0.f)
    {
        const float scale = static_cast<float>(config::tileWidth) / size.x;
        sprite_.setScale({scale, scale});
    }
}

TileType Tile::getType() const
{
    return type_;
}

void Tile::setPosition(sf::Vector2f position)
{
    sprite_.setPosition(position);
}

void Tile::setAlpha(std::uint8_t alpha)
{
    sf::Color color = sprite_.getColor();
    color.a = alpha;
    sprite_.setColor(color);
}

void Tile::draw(sf::RenderTarget& target) const
{
    target.draw(sprite_);
}

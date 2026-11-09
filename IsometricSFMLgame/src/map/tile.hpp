// tile.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>

#include "map/tile_type.hpp"

// A single isometric tile: a sprite plus the type it represents.
//
// The sprite's origin is the centre of its texture, so Map can position a tile
// by simply passing the grid centre returned by Map::gridToScreen(). The sprite
// is scaled so that its width is exactly config::tileWidth pixels, which keeps
// the isometric footprint consistent no matter how large the source art is.
class Tile
{
public:
    Tile(const sf::Texture& texture, TileType type);

    void setType(TileType type);
    [[nodiscard]] TileType getType() const;

    void setPosition(sf::Vector2f position);

    // 0 = fully transparent, 255 = fully opaque.
    void setOpacity(std::uint8_t alpha);

    void draw(sf::RenderTarget& target) const;

private:
    sf::Sprite sprite_;
    TileType type_;
};

// map.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <vector>

#include "map/tile_atlas.hpp"
#include "map/tile_type.hpp"

// A layered isometric tile map.
//
// Coordinate conventions (cells are addressed by their centre):
//   gridToScreen(x, y, z) -> world position of the centre of cell (x, y) at level z
//   screenToGrid(world, z) -> the cell whose centre is closest to `world`
//
// The map owns its tiles but not the textures: every tile is copied from the
// TileAtlas that is passed in.
class Map
{
public:
    // `tileAtlas` must contain TileType::EMPTY and outlive the Map.
    Map(int width, int height, int layers, const TileAtlas& tileAtlas);

    // Places `type` at (x, y, z). TileType::VOID clears the cell instead.
    // Out-of-bounds positions are ignored.
    void placeTile(int x, int y, int z, TileType type);

    // Shows a translucent preview of `type` at `position` (only over empty
    // in-bounds cells). Call updateHover() every frame.
    void updateHover(const sf::Vector3i& position, TileType type);
    void clearHover();

    void draw(sf::RenderTarget& target) const;

    // Draws the grid lines of one z level, aligned with the tile edges.
    void drawOutline(sf::RenderTarget& target, int z) const;
    void drawHover(sf::RenderTarget& target) const;

    [[nodiscard]] sf::Vector2f gridToScreen(int x, int y, int z) const;
    [[nodiscard]] sf::Vector3i screenToGrid(sf::Vector2f screenPosition, int z) const;

    // World position of the centre of the whole map.
    [[nodiscard]] sf::Vector2f getCenter() const;

    [[nodiscard]] int getWidth() const;
    [[nodiscard]] int getHeight() const;
    [[nodiscard]] int getLayers() const;

    [[nodiscard]] bool isInBounds(int x, int y, int z) const;

    // True for empty cells and for out-of-bounds positions.
    [[nodiscard]] bool isEmpty(int x, int y, int z) const;

private:
    [[nodiscard]] std::size_t index(int x, int y, int z) const;

    // Projects fractional grid coordinates, which the grid outline needs.
    [[nodiscard]] sf::Vector2f project(float x, float y, int z) const;

    int width_;
    int height_;
    int layers_;
    std::vector<Tile> tiles_;

    // Painter's-algorithm order, precomputed once because tiles never move.
    std::vector<std::size_t> drawOrder_;

    const TileAtlas& tileAtlas_;
    Tile hoverTile_;
    bool hoverActive_ = false;
};

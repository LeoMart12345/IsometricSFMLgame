// map.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <vector>

#include "map/tile_type.hpp"

// A layered isometric tile map.
//
// Cells are addressed by their centre:
//   gridToScreen(x, y, z) -> world position of the centre of cell (x, y) at level z
//   screenToGrid(world, z) -> the cell whose centre is closest to `world`
//
// Cells store only a TileType; Map draws each one as a coloured diamond.
class Map
{
public:
    Map(int width, int height, int layers);

    // Stores `type` in a cell; TileType::VOID empties the cell again.
    // Out-of-bounds positions are ignored.
    void placeTile(int x, int y, int z, TileType type);

    // Shows a translucent preview of `type` at `position`. Out-of-bounds cells,
    // the eraser and cells that already hold a tile get no preview.
    void updateHover(const sf::Vector3i& position, TileType type);

    // Draws back to front, so nearer cells cover the cells behind them.
    void draw(sf::RenderTarget& target) const;
    void drawHover(sf::RenderTarget& target) const;

    // The grid lines of one z level, running along the cell borders.
    void drawOutline(sf::RenderTarget& target, int z) const;

    [[nodiscard]] sf::Vector2f gridToScreen(int x, int y, int z) const;
    [[nodiscard]] sf::Vector3i screenToGrid(sf::Vector2f worldPosition, int z) const;

    // World position of the centre of the whole map.
    [[nodiscard]] sf::Vector2f getCenter() const;

    [[nodiscard]] int getLayers() const;

private:
    [[nodiscard]] bool isInBounds(int x, int y, int z) const;

    // Position of a cell in the flat tiles_ vector.
    [[nodiscard]] std::size_t index(int x, int y, int z) const;

    // Converts a flat index back to x, y, z and returns the painter's depth
    // (x + y + z) used for draw ordering.
    [[nodiscard]] int depthOf(std::size_t tileIndex) const;

    // Projects fractional grid coordinates, which drawOutline() needs because
    // cell borders sit half a cell away from the cell centres.
    [[nodiscard]] sf::Vector2f screenPosition(float x, float y, int z) const;

    int width_;
    int height_;
    int layers_;
    std::vector<TileType> tiles_;
    std::vector<sf::Vector2f> positions_;

    // Painter's-algorithm order, precomputed once because tiles never move.
    std::vector<std::size_t> drawOrder_;

    sf::Vector3i hoverCell_{-1, -1, -1};
    TileType hoverType_ = TileType::EMPTY;
    bool hoverActive_ = false;
};

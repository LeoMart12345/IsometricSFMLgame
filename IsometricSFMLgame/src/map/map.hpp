// map.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <vector>

#include "map/tile_atlas.hpp"
#include "map/tile_type.hpp"

// A layered isometric tile map.
//
// Cells are addressed by their centre:
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

    // Stores `type` in a cell; TileType::VOID empties the cell again.
    // Out-of-bounds positions are ignored, as are types the atlas does not know.
    void placeTile(int x, int y, int z, TileType type);

    // Shows a translucent preview of `type` at `position`. Only empty in-bounds
    // cells get a preview, so calling this every frame also hides a stale one.
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

    [[nodiscard]] int getWidth() const;
    [[nodiscard]] int getHeight() const;
    [[nodiscard]] int getLayers() const;

    [[nodiscard]] bool isInBounds(int x, int y, int z) const;

private:
    // Position of a cell in the flat tiles_ vector.
    [[nodiscard]] std::size_t index(int x, int y, int z) const;

    // Converts a flat index back to x, y, z and returns the painter's depth
    // (x + y + z) used for draw ordering.
    [[nodiscard]] int depthOf(std::size_t tileIndex) const;

    // Projects fractional grid coordinates, which drawOutline() needs because
    // cell borders sit half a cell away from the cell centres.
    [[nodiscard]] sf::Vector2f screenPosition(float x, float y, int z) const;

    // True for empty cells and for out-of-bounds positions.
    [[nodiscard]] bool isEmpty(int x, int y, int z) const;

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

// map.cpp
#include "map/map.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "core/constants.hpp"
#include "map/grid_geometry.hpp"

namespace
{
constexpr sf::Color outlineColor{255, 255, 255, 60};

// One tile, centred on the origin.
sf::ConvexShape makeDiamond()
{
    sf::ConvexShape diamond(4);
    diamond.setPoint(0, {0.f, -iso::halfTileHeight});
    diamond.setPoint(1, {iso::halfTileWidth, 0.f});
    diamond.setPoint(2, {0.f, iso::halfTileHeight});
    diamond.setPoint(3, {-iso::halfTileWidth, 0.f});
    return diamond;
}

sf::Color colorOf(TileType type)
{
    switch (type)
    {
    case TileType::GRASS:
        return {110, 190, 90};
    case TileType::FOREST:
        return {40, 110, 60};
    case TileType::WATER:
        return {70, 130, 220};
    case TileType::RESIDENTIAL:
        return {220, 180, 90};
    case TileType::COMMERCIAL:
        return {160, 90, 220};
    case TileType::INDUSTRIAL:
        return {130, 130, 130};
    case TileType::ROAD:
        return {80, 80, 90};
    case TileType::VOID:
    case TileType::EMPTY:
        return sf::Color::Transparent;
    }

    return sf::Color::Transparent;
}
} // namespace

Map::Map(int width, int height, int layers) : width_(width), height_(height), layers_(layers)
{
    if (width_ <= 0 || height_ <= 0 || layers_ <= 0)
        throw std::invalid_argument("Map dimensions must be positive");

    const std::size_t tileCount = static_cast<std::size_t>(width_) * height_ * layers_;

    // Every cell starts empty; placeTile() fills the cells the editor touches.
    tiles_.assign(tileCount, TileType::EMPTY);
    positions_.resize(tileCount);

    for (int z = 0; z < layers_; ++z)
        for (int y = 0; y < height_; ++y)
            for (int x = 0; x < width_; ++x)
                positions_[index(x, y, z)] = gridToScreen(x, y, z);

    // Tiles never move and never change depth, so the painter's order (back to
    // front, by increasing x + y + z) is computed once here. A stable sort
    // keeps equally deep cells in index order.
    drawOrder_.resize(tileCount);
    std::iota(drawOrder_.begin(), drawOrder_.end(), std::size_t{0});
    std::stable_sort(drawOrder_.begin(), drawOrder_.end(),
                     [this](std::size_t lhs, std::size_t rhs) { return depthOf(lhs) < depthOf(rhs); });
}

void Map::placeTile(int x, int y, int z, TileType type)
{
    if (!isInBounds(x, y, z))
        return;

    // VOID is the editor's eraser, so it stores the empty cell instead.
    tiles_[index(x, y, z)] = (type == TileType::VOID) ? TileType::EMPTY : type;
}

void Map::updateHover(const sf::Vector3i& position, TileType type)
{
    // The preview is only useful over an empty in-bounds cell; the eraser has
    // nothing to preview.
    hoverActive_ = type != TileType::VOID && isInBounds(position.x, position.y, position.z) &&
                   tiles_[index(position.x, position.y, position.z)] == TileType::EMPTY;
    hoverCell_ = position;
    hoverType_ = type;
}

void Map::draw(sf::RenderTarget& target) const
{
    sf::ConvexShape diamond = makeDiamond();

    for (const std::size_t tileIndex : drawOrder_)
    {
        const TileType type = tiles_[tileIndex];
        if (type == TileType::EMPTY)
            continue;

        diamond.setPosition(positions_[tileIndex]);
        diamond.setFillColor(colorOf(type));
        target.draw(diamond);
    }
}

void Map::drawHover(sf::RenderTarget& target) const
{
    if (!hoverActive_)
        return;

    sf::ConvexShape diamond = makeDiamond();
    sf::Color color = colorOf(hoverType_);
    color.a = config::hoverAlpha;

    diamond.setPosition(gridToScreen(hoverCell_.x, hoverCell_.y, hoverCell_.z));
    diamond.setFillColor(color);
    target.draw(diamond);
}

void Map::drawOutline(sf::RenderTarget& target, int z) const
{
    sf::VertexArray lines(sf::PrimitiveType::Lines);

    const auto addLine = [&lines](sf::Vector2f from, sf::Vector2f to)
    {
        lines.append(sf::Vertex{from, outlineColor});
        lines.append(sf::Vertex{to, outlineColor});
    };

    // The borders sit half a cell away from the cell centres, so the grid runs
    // from -0.5 to width_ - 0.5 (and likewise for y).
    const float left = -0.5f;
    const float top = -0.5f;
    const float right = static_cast<float>(width_) - 0.5f;
    const float bottom = static_cast<float>(height_) - 0.5f;

    for (int x = 0; x <= width_; ++x)
    {
        const float border = static_cast<float>(x) - 0.5f;
        addLine(screenPosition(border, top, z), screenPosition(border, bottom, z));
    }

    for (int y = 0; y <= height_; ++y)
    {
        const float border = static_cast<float>(y) - 0.5f;
        addLine(screenPosition(left, border, z), screenPosition(right, border, z));
    }

    target.draw(lines);
}

sf::Vector2f Map::gridToScreen(int x, int y, int z) const
{
    return screenPosition(static_cast<float>(x), static_cast<float>(y), z);
}

sf::Vector3i Map::screenToGrid(sf::Vector2f worldPosition, int z) const
{
    const iso::PointF grid = iso::unproject({worldPosition.x, worldPosition.y}, static_cast<float>(z));
    const iso::GridPoint cell = iso::snap(grid);

    return {cell.x, cell.y, z};
}

sf::Vector2f Map::getCenter() const
{
    return screenPosition((static_cast<float>(width_) - 1.f) / 2.f, (static_cast<float>(height_) - 1.f) / 2.f, 0);
}

int Map::getLayers() const
{
    return layers_;
}

bool Map::isInBounds(int x, int y, int z) const
{
    return x >= 0 && x < width_ && y >= 0 && y < height_ && z >= 0 && z < layers_;
}

std::size_t Map::index(int x, int y, int z) const
{
    return (static_cast<std::size_t>(z) * height_ + y) * width_ + x;
}

int Map::depthOf(std::size_t tileIndex) const
{
    // Inverse of index(): x varies fastest, z slowest.
    const auto w = static_cast<std::size_t>(width_);
    const auto h = static_cast<std::size_t>(height_);

    return static_cast<int>(tileIndex % w + (tileIndex / w) % h + tileIndex / (w * h));
}

sf::Vector2f Map::screenPosition(float x, float y, int z) const
{
    const iso::PointF screen = iso::project(x, y, static_cast<float>(z));

    return {screen.x, screen.y};
}

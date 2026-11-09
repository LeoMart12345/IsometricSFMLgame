// map.cpp
#include "map/map.hpp"

#include <algorithm>
#include <stdexcept>

#include "core/constants.hpp"
#include "map/grid_geometry.hpp"

namespace
{
const Tile& emptyTile(const TileAtlas& atlas)
{
    const auto it = atlas.find(TileType::EMPTY);
    if (it == atlas.end())
        throw std::invalid_argument("Map requires a tile atlas that contains TileType::EMPTY");

    return it->second;
}

sf::Color outlineColor()
{
    return sf::Color(255, 255, 255, 60);
}
} // namespace

Map::Map(int width, int height, int layers, const TileAtlas& tileAtlas)
    : width_(width), height_(height), layers_(layers), tileAtlas_(tileAtlas), hoverTile_(emptyTile(tileAtlas))
{
    if (width_ <= 0 || height_ <= 0 || layers_ <= 0)
        throw std::invalid_argument("Map dimensions must be positive");

    const std::size_t tileCount = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) *
                                  static_cast<std::size_t>(layers_);

    tiles_.reserve(tileCount);
    for (std::size_t i = 0; i < tileCount; ++i)
        tiles_.push_back(emptyTile(tileAtlas_));

    // Every tile is positioned once and never moves again, so the painter's
    // order can be computed here instead of every frame.
    drawOrder_.resize(tileCount);
    for (std::size_t i = 0; i < tileCount; ++i)
        drawOrder_[i] = i;

    const int w = width_;
    const int h = height_;

    std::sort(drawOrder_.begin(), drawOrder_.end(),
              [w, h](std::size_t lhs, std::size_t rhs)
              {
                  const int lhsDepth = static_cast<int>(lhs % static_cast<std::size_t>(w)) +
                                       static_cast<int>((lhs / static_cast<std::size_t>(w)) % static_cast<std::size_t>(h)) +
                                       static_cast<int>(lhs / (static_cast<std::size_t>(w) * static_cast<std::size_t>(h)));
                  const int rhsDepth = static_cast<int>(rhs % static_cast<std::size_t>(w)) +
                                       static_cast<int>((rhs / static_cast<std::size_t>(w)) % static_cast<std::size_t>(h)) +
                                       static_cast<int>(rhs / (static_cast<std::size_t>(w) * static_cast<std::size_t>(h)));

                  if (lhsDepth != rhsDepth)
                      return lhsDepth < rhsDepth;

                  return lhs < rhs;
              });

    for (int z = 0; z < layers_; ++z)
        for (int y = 0; y < height_; ++y)
            for (int x = 0; x < width_; ++x)
                tiles_[index(x, y, z)].setPosition(gridToScreen(x, y, z));
}

void Map::placeTile(int x, int y, int z, TileType type)
{
    if (!isInBounds(x, y, z))
        return;

    const TileType storedType = (type == TileType::VOID) ? TileType::EMPTY : type;

    auto prototype = tileAtlas_.find(storedType);
    if (prototype == tileAtlas_.end())
        prototype = tileAtlas_.find(TileType::EMPTY);
    if (prototype == tileAtlas_.end())
        return;

    Tile& tile = tiles_[index(x, y, z)];
    if (tile.getType() == storedType)
        return;

    tile = prototype->second;
    tile.setPosition(gridToScreen(x, y, z));
}

void Map::updateHover(const sf::Vector3i& position, TileType type)
{
    if (type == TileType::VOID || !isEmpty(position.x, position.y, position.z))
    {
        clearHover();
        return;
    }

    const auto prototype = tileAtlas_.find(type);
    if (prototype == tileAtlas_.end())
    {
        clearHover();
        return;
    }

    hoverTile_ = prototype->second;
    hoverTile_.setPosition(gridToScreen(position.x, position.y, position.z));
    hoverTile_.setOpacity(config::hoverAlpha);
    hoverActive_ = true;
}

void Map::clearHover()
{
    hoverActive_ = false;
}

void Map::draw(sf::RenderTarget& target) const
{
    for (const std::size_t tileIndex : drawOrder_)
    {
        const Tile& tile = tiles_[tileIndex];
        if (tile.getType() != TileType::EMPTY)
            tile.draw(target);
    }
}

void Map::drawOutline(sf::RenderTarget& target, int z) const
{
    sf::VertexArray grid(sf::PrimitiveType::Lines);

    // The grid lines run along cell borders, which sit half a cell away from
    // the cell centres returned by gridToScreen().
    for (int x = 0; x <= width_; ++x)
    {
        grid.append(sf::Vertex{project(static_cast<float>(x) - 0.5f, -0.5f, z), outlineColor()});
        grid.append(sf::Vertex{project(static_cast<float>(x) - 0.5f, static_cast<float>(height_) - 0.5f, z),
                               outlineColor()});
    }

    for (int y = 0; y <= height_; ++y)
    {
        grid.append(sf::Vertex{project(-0.5f, static_cast<float>(y) - 0.5f, z), outlineColor()});
        grid.append(sf::Vertex{project(static_cast<float>(width_) - 0.5f, static_cast<float>(y) - 0.5f, z),
                               outlineColor()});
    }

    target.draw(grid);
}

void Map::drawHover(sf::RenderTarget& target) const
{
    if (hoverActive_)
        hoverTile_.draw(target);
}

sf::Vector2f Map::gridToScreen(int x, int y, int z) const
{
    return project(static_cast<float>(x), static_cast<float>(y), z);
}

sf::Vector3i Map::screenToGrid(sf::Vector2f screenPosition, int z) const
{
    const iso::PointF grid = iso::unproject({screenPosition.x, screenPosition.y}, static_cast<float>(z));
    const iso::GridPoint cell = iso::snap(grid);

    return {cell.x, cell.y, z};
}

sf::Vector2f Map::getCenter() const
{
    return project((static_cast<float>(width_) - 1.f) / 2.f, (static_cast<float>(height_) - 1.f) / 2.f, 0);
}

int Map::getWidth() const
{
    return width_;
}

int Map::getHeight() const
{
    return height_;
}

int Map::getLayers() const
{
    return layers_;
}

bool Map::isInBounds(int x, int y, int z) const
{
    return x >= 0 && x < width_ && y >= 0 && y < height_ && z >= 0 && z < layers_;
}

bool Map::isEmpty(int x, int y, int z) const
{
    if (!isInBounds(x, y, z))
        return true;

    return tiles_[index(x, y, z)].getType() == TileType::EMPTY;
}

std::size_t Map::index(int x, int y, int z) const
{
    return (static_cast<std::size_t>(z) * static_cast<std::size_t>(height_) + static_cast<std::size_t>(y)) *
               static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(x);
}

sf::Vector2f Map::project(float x, float y, int z) const
{
    const iso::PointF screen = iso::project(x, y, static_cast<float>(z));

    return {screen.x, screen.y};
}

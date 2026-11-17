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

constexpr sf::Color outlineColor{255, 255, 255, 60};
} // namespace

Map::Map(int width, int height, int layers, const TileAtlas& tileAtlas)
    : width_(width), height_(height), layers_(layers), tileAtlas_(tileAtlas), hoverTile_(emptyTile(tileAtlas))
{
    if (width_ <= 0 || height_ <= 0 || layers_ <= 0)
        throw std::invalid_argument("Map dimensions must be positive");

    const std::size_t tileCount =
        static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * static_cast<std::size_t>(layers_);

    // Start with every cell empty; placeTile() overwrites the cells the editor
    // fills in later.
    tiles_.assign(tileCount, emptyTile(tileAtlas_));

    // Tiles never move and never change depth, so the painter's order is
    // computed once here rather than every frame. Sorting by x + y + z draws
    // cells that are further back first, so nearer cells cover them. A stable
    // sort keeps equally deep cells in index order, which is all the
    // tie-breaking this needs.
    drawOrder_.resize(tileCount);
    for (std::size_t i = 0; i < tileCount; ++i)
        drawOrder_[i] = i;

    std::stable_sort(drawOrder_.begin(), drawOrder_.end(),
                     [this](std::size_t lhs, std::size_t rhs) { return depthOf(lhs) < depthOf(rhs); });

    for (int z = 0; z < layers_; ++z)
        for (int y = 0; y < height_; ++y)
            for (int x = 0; x < width_; ++x)
                tiles_[index(x, y, z)].setPosition(gridToScreen(x, y, z));
}

void Map::placeTile(int x, int y, int z, TileType type)
{
    if (!isInBounds(x, y, z))
        return;

    // VOID is the editor's eraser, so it stores the empty tile instead.
    const TileType storedType = (type == TileType::VOID) ? TileType::EMPTY : type;

    // The atlas defines which types exist; anything unknown leaves the cell as
    // it is.
    const auto prototype = tileAtlas_.find(storedType);
    if (prototype == tileAtlas_.end())
        return;

    Tile& tile = tiles_[index(x, y, z)];
    if (tile.getType() == storedType)
        return;

    // Copy the prototype so the atlas keeps its untouched template.
    tile = prototype->second;
    tile.setPosition(gridToScreen(x, y, z));
}

void Map::updateHover(const sf::Vector3i& position, TileType type)
{
    // The cell has to be in bounds: isEmpty() reports out-of-bounds cells as
    // empty, so without the bounds check the preview would float outside the
    // map. VOID is the eraser and never gets a preview.
    const auto prototype = tileAtlas_.find(type);
    const bool canPlace = isInBounds(position.x, position.y, position.z) && type != TileType::VOID &&
                          isEmpty(position.x, position.y, position.z) && prototype != tileAtlas_.end();

    hoverActive_ = canPlace;
    if (!canPlace)
        return;

    hoverTile_ = prototype->second;
    hoverTile_.setPosition(gridToScreen(position.x, position.y, position.z));
    hoverTile_.setAlpha(config::hoverAlpha);
}

void Map::draw(sf::RenderTarget& target) const
{
    for (const std::size_t tileIndex : drawOrder_)
    {
        const Tile& tile = tiles_[tileIndex];

        // Empty cells only hold transparent placeholder art, so skipping them
        // saves a draw call per unfilled cell.
        if (tile.getType() != TileType::EMPTY)
            tile.draw(target);
    }
}

void Map::drawHover(sf::RenderTarget& target) const
{
    if (hoverActive_)
        hoverTile_.draw(target);
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

std::size_t Map::index(int x, int y, int z) const
{
    return (static_cast<std::size_t>(z) * static_cast<std::size_t>(height_) + static_cast<std::size_t>(y)) *
               static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(x);
}

int Map::depthOf(std::size_t tileIndex) const
{
    // Inverse of index(): x varies fastest, z slowest.
    const std::size_t w = static_cast<std::size_t>(width_);
    const std::size_t h = static_cast<std::size_t>(height_);

    const int x = static_cast<int>(tileIndex % w);
    const int y = static_cast<int>((tileIndex / w) % h);
    const int z = static_cast<int>(tileIndex / (w * h));

    return x + y + z;
}

sf::Vector2f Map::screenPosition(float x, float y, int z) const
{
    const iso::PointF screen = iso::project(x, y, static_cast<float>(z));

    return {screen.x, screen.y};
}

bool Map::isEmpty(int x, int y, int z) const
{
    if (!isInBounds(x, y, z))
        return true;

    return tiles_[index(x, y, z)].getType() == TileType::EMPTY;
}


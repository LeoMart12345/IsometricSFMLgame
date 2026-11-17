// tile_grid.cpp
#include "map/tile_grid.hpp"

#include <limits>
#include <stdexcept>

#include "core/assert.hpp"

namespace
{
// Only used inside ISO_ASSERT_MSG, which compiles away in release builds.
[[maybe_unused]] std::string describe(int x, int y, int z)
{
    return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
}
} // namespace

TileGrid::TileGrid(int width, int height, int layers, TileType fill)
    : width_(width), height_(height), layers_(layers)
{
    if (width <= 0 || height <= 0 || layers <= 0)
        throw std::invalid_argument("TileGrid needs positive dimensions, got " + std::to_string(width) +
                                    "x" + std::to_string(height) + "x" + std::to_string(layers));

    const std::size_t count = cellCount();
    if (count > cells_.max_size())
        throw std::length_error("TileGrid is too large: " + std::to_string(width) + "x" +
                                std::to_string(height) + "x" + std::to_string(layers));

    cells_.assign(count, fill == TileType::VOID ? TileType::EMPTY : fill);
}

std::size_t TileGrid::cellCount() const
{
    // Checked multiplication: three int dimensions can overflow std::size_t on
    // 64 bit (INT_MAX^3 needs 93 bits), and a wrapped count would allocate a
    // small vector for a grid that still claims gigantic dimensions. That turned
    // every later index() into an out-of-bounds write.
    constexpr std::size_t kMax = std::numeric_limits<std::size_t>::max();
    const auto width = static_cast<std::size_t>(width_);
    const auto height = static_cast<std::size_t>(height_);
    const auto layers = static_cast<std::size_t>(layers_);

    if (width != 0 && height > kMax / width)
        throw std::length_error("TileGrid dimensions overflow the addressable cell count");

    const std::size_t layerSize = width * height;
    if (layers != 0 && layerSize > kMax / layers)
        throw std::length_error("TileGrid dimensions overflow the addressable cell count");

    return layerSize * layers;
}

bool TileGrid::isInBounds(int x, int y, int z) const
{
    return x >= 0 && x < width_ && y >= 0 && y < height_ && z >= 0 && z < layers_;
}

std::size_t TileGrid::index(int x, int y, int z) const
{
    ISO_ASSERT_MSG(isInBounds(x, y, z), ("TileGrid::index out of bounds " + describe(x, y, z)).c_str());

    return (static_cast<std::size_t>(z) * static_cast<std::size_t>(height_) +
            static_cast<std::size_t>(y)) *
               static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(x);
}

TileType TileGrid::at(int x, int y, int z) const
{
    return cells_[index(x, y, z)];
}

bool TileGrid::set(int x, int y, int z, TileType type)
{
    if (!isInBounds(x, y, z))
        return false;

    const std::size_t cell = index(x, y, z);
    const TileType stored = (type == TileType::VOID) ? TileType::EMPTY : type;
    if (cells_[cell] == stored)
        return false;

    cells_[cell] = stored;
    return true;
}

void TileGrid::store(int x, int y, int z, TileType type)
{
    ISO_ASSERT(isInBounds(x, y, z));
    cells_[index(x, y, z)] = (type == TileType::VOID) ? TileType::EMPTY : type;
}

void TileGrid::fill(TileType type)
{
    cells_.assign(cellCount(), type == TileType::VOID ? TileType::EMPTY : type);
}

bool TileGrid::isEmpty(int x, int y, int z) const
{
    return isInBounds(x, y, z) && cells_[index(x, y, z)] == TileType::EMPTY;
}

bool TileGrid::isWalkable(int x, int y, int z) const
{
    // Qualified: the member function hides the free isWalkable(TileType).
    return isInBounds(x, y, z) && ::isWalkable(cells_[index(x, y, z)]);
}

int TileGrid::moveCost(int x, int y, int z) const
{
    return isInBounds(x, y, z) ? ::moveCost(cells_[index(x, y, z)]) : 0;
}

bool TileGrid::validate(std::string* reason) const
{
    const auto fail = [reason](const std::string& message) {
        if (reason != nullptr)
            *reason = message;
        return false;
    };

    if (width_ <= 0 || height_ <= 0 || layers_ <= 0)
        return fail("grid dimensions must be positive");

    if (cells_.size() != cellCount())
        return fail("cell storage size does not match the grid dimensions");

    for (std::size_t i = 0; i < cells_.size(); ++i)
    {
        const TileType type = cells_[i];
        if (type == TileType::VOID)
            return fail("VOID must never be stored in a grid (cell " + std::to_string(i) + ")");

        if (!isValidTileType(type))
            return fail("unknown tile type stored at cell " + std::to_string(i));

        const TileProperties properties = tileProperties(type);
        if (properties.walkable != (properties.moveCost >= 1))
            return fail("inconsistent tile properties for " + tileTypeToStr(type));
    }

    if (reason != nullptr)
        reason->clear();
    return true;
}

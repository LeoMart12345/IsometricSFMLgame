// grid_geometry.hpp
#pragma once

#include <cmath>

#include "core/constants.hpp"

// Pure isometric projection math. It deliberately has no SFML dependency so it
// can be unit tested (see tests/grid_geometry_test.cpp).
//
// Convention: grid coordinates address the *centre* of a cell.
//   screen.x = (x - y) * tileWidth  / 2
//   screen.y = (x + y) * tileHeight / 2 - z * tileHeight / 2
//
// Because the projection is linear and invertible, a screen point inside a
// tile's diamond maps back to grid coordinates within +-0.5 of that tile's
// centre, which is what snap() exploits to pick the clicked cell.
namespace iso
{
struct PointF
{
    float x;
    float y;
};

struct GridPoint
{
    int x;
    int y;
};

inline constexpr float halfTileWidth = static_cast<float>(config::tileWidth) / 2.f;
inline constexpr float halfTileHeight = static_cast<float>(config::tileHeight) / 2.f;

[[nodiscard]] constexpr PointF project(float x, float y, float z)
{
    return {(x - y) * halfTileWidth, (x + y) * halfTileHeight - z * halfTileHeight};
}

// Inverse of project() for a known z level.
[[nodiscard]] constexpr PointF unproject(PointF screen, float z)
{
    const float adjustedY = screen.y + z * halfTileHeight;
    return {((screen.x / halfTileWidth) + (adjustedY / halfTileHeight)) / 2.f,
            ((adjustedY / halfTileHeight) - (screen.x / halfTileWidth)) / 2.f};
}

// The cell whose centre is closest to `grid`.
[[nodiscard]] inline GridPoint snap(PointF grid)
{
    // lround() (not a truncating cast) so clicks left/above the origin work.
    return {static_cast<int>(std::lround(grid.x)), static_cast<int>(std::lround(grid.y))};
}
} // namespace iso

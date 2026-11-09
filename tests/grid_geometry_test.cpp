// tests/grid_geometry_test.cpp
//
// Unit tests for the isometric projection used by Map. The header under test
// has no SFML dependency, so these run without a graphics context.
//
//   cmake -S . -B build -DISO_BUILD_GAME=OFF && cmake --build build
//   ctest --test-dir build --output-on-failure
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include "map/grid_geometry.hpp"

namespace
{
int g_failures = 0;

void check(bool condition, const std::string& message)
{
    if (!condition)
    {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) < 0.0001f;
}

void checkPoint(iso::PointF actual, iso::PointF expected, const std::string& message)
{
    check(nearlyEqual(actual.x, expected.x) && nearlyEqual(actual.y, expected.y), message);
}

// A click on the centre of a tile must select that exact tile, for every layer.
void testRoundTrip()
{
    for (int z = 0; z < static_cast<int>(config::mapLayers); ++z)
    {
        for (int y = -12; y <= 12; ++y)
        {
            for (int x = -12; x <= 12; ++x)
            {
                const iso::PointF screen =
                    iso::project(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                const iso::GridPoint cell = iso::snap(iso::unproject(screen, static_cast<float>(z)));

                check(cell.x == x && cell.y == y, "round trip failed at (" + std::to_string(x) + ", " +
                                                      std::to_string(y) + ", " + std::to_string(z) + ")");
            }
        }
    }
}

// Any point inside a tile's diamond must belong to that tile (this is what
// makes the hover preview and the placement click agree).
void testClicksInsideDiamond()
{
    constexpr float kInsideOffsets[] = {-0.45f, 0.0f, 0.45f};

    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            for (const float offsetX : kInsideOffsets)
            {
                for (const float offsetY : kInsideOffsets)
                {
                    // The projection is linear, so the diamond of a tile maps
                    // to the unit square of the grid.
                    if (std::max(std::fabs(offsetX), std::fabs(offsetY)) > 0.5f)
                        continue;

                    const iso::PointF screen =
                        iso::project(static_cast<float>(x) + offsetX, static_cast<float>(y) + offsetY, 0.f);
                    const iso::GridPoint cell = iso::snap(iso::unproject(screen, 0.f));

                    check(cell.x == x && cell.y == y,
                          "click inside tile (" + std::to_string(x) + ", " + std::to_string(y) + ") selected (" +
                              std::to_string(cell.x) + ", " + std::to_string(cell.y) + ")");
                }
            }
        }
    }

    // A click clearly outside the diamond must not select the centre tile.
    const iso::PointF centre = iso::project(0.f, 0.f, 0.f);
    const iso::PointF outside{centre.x + iso::halfTileWidth * 0.9f, centre.y + iso::halfTileHeight * 0.9f};
    const iso::GridPoint outsideCell = iso::snap(iso::unproject(outside, 0.f));
    check(!(outsideCell.x == 0 && outsideCell.y == 0), "click outside the diamond selected the centre tile");
}

// Neighbouring cells are exactly half a tile apart in both axes.
void testNeighbourSpacing()
{
    const iso::PointF origin = iso::project(0.f, 0.f, 0.f);
    const iso::PointF right = iso::project(1.f, 0.f, 0.f);
    const iso::PointF down = iso::project(0.f, 1.f, 0.f);

    checkPoint(right, {origin.x + iso::halfTileWidth, origin.y + iso::halfTileHeight},
               "cell (1, 0) is not down-right of cell (0, 0)");
    checkPoint(down, {origin.x - iso::halfTileWidth, origin.y + iso::halfTileHeight},
               "cell (0, 1) is not down-left of cell (0, 0)");
}

// Increasing z moves a tile up by half a tile height.
void testZShift()
{
    const iso::PointF ground = iso::project(3.f, 2.f, 0.f);
    const iso::PointF raised = iso::project(3.f, 2.f, 1.f);

    checkPoint(raised, {ground.x, ground.y - iso::halfTileHeight}, "z=1 is not half a tile above z=0");
}

// The grid outline drawn by Map::drawOutline() must end on the diamond corners
// of the map, otherwise the grid and the tiles disagree by half a tile.
void testOutlineCorners()
{
    const float width = static_cast<float>(config::mapWidth);
    const float height = static_cast<float>(config::mapHeight);

    const iso::PointF topLeftTileCentre = iso::project(0.f, 0.f, 0.f);
    const iso::PointF topRightTileCentre = iso::project(width - 1.f, 0.f, 0.f);
    const iso::PointF bottomLeftTileCentre = iso::project(0.f, height - 1.f, 0.f);

    // First vertical grid line (x == 0) starts at the top vertex of tile (0, 0).
    checkPoint(iso::project(-0.5f, -0.5f, 0.f), {topLeftTileCentre.x, topLeftTileCentre.y - iso::halfTileHeight},
               "top-left grid corner is not the top vertex of tile (0, 0)");

    // Last vertical grid line (x == width) starts at the right vertex of the
    // top-right tile.
    checkPoint(iso::project(width - 0.5f, -0.5f, 0.f),
               {topRightTileCentre.x + iso::halfTileWidth, topRightTileCentre.y},
               "top-right grid corner is not the right vertex of the top-right tile");

    // The first vertical grid line ends at the left vertex of tile (0, height-1).
    checkPoint(iso::project(-0.5f, height - 0.5f, 0.f),
               {bottomLeftTileCentre.x - iso::halfTileWidth, bottomLeftTileCentre.y},
               "bottom-left grid corner is not the left vertex of the bottom-left tile");

    // The outline follows the current layer.
    checkPoint(iso::project(-0.5f, -0.5f, 2.f),
               {topLeftTileCentre.x, topLeftTileCentre.y - iso::halfTileHeight * 3.f},
               "grid outline does not follow the z level");
}
} // namespace

int main()
{
    testRoundTrip();
    testClicksInsideDiamond();
    testNeighbourSpacing();
    testZShift();
    testOutlineCorners();

    if (g_failures != 0)
    {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }

    std::cout << "All grid geometry checks passed\n";
    return 0;
}

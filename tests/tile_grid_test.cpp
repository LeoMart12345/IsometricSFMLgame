// tile_grid_test.cpp
//
// Rules that the rest of the game relies on: bounds behaviour, the VOID
// command, change detection, and TileType -> TileProperties consistency.
#include <set>
#include <string>
#include <vector>

#include "core/rng.hpp"
#include "map/tile_grid.hpp"
#include "map/tile_properties.hpp"
#include "map/tile_type.hpp"
#include "test_framework.hpp"

ISO_TEST(new_grid_is_empty_and_validates)
{
    const TileGrid grid(4, 3, 2);

    CHECK_EQ(grid.width(), 4);
    CHECK_EQ(grid.height(), 3);
    CHECK_EQ(grid.layers(), 2);
    CHECK_EQ(grid.cellCount(), static_cast<std::size_t>(4 * 3 * 2));

    std::string reason;
    CHECK_NOTE(grid.validate(&reason), reason);

    for (int z = 0; z < grid.layers(); ++z)
        for (int y = 0; y < grid.height(); ++y)
            for (int x = 0; x < grid.width(); ++x)
                CHECK_NOTE(grid.at(x, y, z) == TileType::EMPTY, reason);
}

ISO_TEST(invalid_dimensions_are_rejected)
{
    CHECK_THROWS(TileGrid(0, 1, 1));
    CHECK_THROWS(TileGrid(1, 0, 1));
    CHECK_THROWS(TileGrid(1, 1, 0));
    CHECK_THROWS(TileGrid(-3, 4, 1));
}

ISO_TEST(bounds_checks_cover_every_border)
{
    const TileGrid grid(2, 2, 2);

    CHECK(grid.isInBounds(0, 0, 0));
    CHECK(grid.isInBounds(1, 1, 1));
    CHECK(!grid.isInBounds(-1, 0, 0));
    CHECK(!grid.isInBounds(0, -1, 0));
    CHECK(!grid.isInBounds(0, 0, -1));
    CHECK(!grid.isInBounds(2, 0, 0));
    CHECK(!grid.isInBounds(0, 2, 0));
    CHECK(!grid.isInBounds(0, 0, 2));

    // Queries outside the grid answer "nothing here" instead of crashing.
    CHECK(!grid.isEmpty(-1, 0, 0));
    CHECK(!grid.isWalkable(0, 0, 9));
    CHECK_EQ(grid.moveCost(-5, -5, 0), 0);

    // A cell that exists but has no floor is empty but not walkable.
    CHECK(grid.isEmpty(1, 1, 1));
    CHECK(!grid.isWalkable(1, 1, 1));
}

ISO_TEST(store_and_set_behave_consistently)
{
    TileGrid grid(3, 3, 1);

    CHECK(grid.set(1, 1, 0, TileType::GRASS));
    CHECK_EQ(grid.at(1, 1, 0) == TileType::GRASS, true);

    // Setting the same type again reports "nothing changed".
    CHECK(!grid.set(1, 1, 0, TileType::GRASS));

    // VOID is a command: it clears the cell and never ends up stored.
    CHECK(grid.set(1, 1, 0, TileType::VOID));
    CHECK_EQ(grid.at(1, 1, 0) == TileType::EMPTY, true);

    // Out-of-bounds writes are ignored.
    CHECK(!grid.set(-1, 0, 0, TileType::GRASS));
    CHECK(!grid.set(0, 0, 1, TileType::GRASS));

    // store() bypasses the change detection but keeps the same normalisation.
    grid.store(2, 2, 0, TileType::VOID);
    CHECK_EQ(grid.at(2, 2, 0) == TileType::EMPTY, true);

    std::string reason;
    CHECK_NOTE(grid.validate(&reason), reason);
}

ISO_TEST(flush_layer_is_independent_of_other_layers)
{
    TileGrid grid(2, 2, 3);
    grid.fill(TileType::GRASS);

    grid.set(0, 0, 1, TileType::WATER);

    CHECK_EQ(grid.at(0, 0, 0) == TileType::GRASS, true);
    CHECK_EQ(grid.at(0, 0, 1) == TileType::WATER, true);
    CHECK_EQ(grid.at(0, 0, 2) == TileType::GRASS, true);

    CHECK(!grid.isWalkable(0, 0, 1));
    CHECK(grid.isWalkable(0, 0, 2));
}

ISO_TEST(fill_normalises_void)
{
    TileGrid grid(2, 2, 1);
    grid.fill(TileType::VOID);

    std::string reason;
    CHECK_NOTE(grid.validate(&reason), reason);
    CHECK_EQ(grid.at(0, 0, 0) == TileType::EMPTY, true);
}

ISO_TEST(tile_properties_are_consistent_for_every_type)
{
    for (const TileType type : allTileTypes)
    {
        const TileProperties properties = tileProperties(type);
        CHECK_EQ(properties.walkable, properties.moveCost >= 1);

        if (isPlaceable(type))
            CHECK_NOTE(properties.moveCost >= 1 || !properties.walkable, tileTypeToStr(type));
    }

    // The rules the game design depends on.
    CHECK(isWalkable(TileType::GRASS));
    CHECK(isWalkable(TileType::ROAD));
    CHECK(!isWalkable(TileType::WATER));
    CHECK(!isWalkable(TileType::EMPTY));
    CHECK(!isWalkable(TileType::VOID));

    // Woods are slower than open ground, roads are not.
    CHECK(moveCost(TileType::FOREST) > moveCost(TileType::GRASS));
    CHECK_EQ(moveCost(TileType::ROAD), moveCost(TileType::GRASS));
}

ISO_TEST(every_tile_type_has_a_unique_name)
{
    std::set<std::string> names;
    std::set<std::string> textures;

    for (const TileType type : allTileTypes)
    {
        names.insert(tileTypeToStr(type));

        // VOID shares EMPTY's texture on purpose (clearing a cell restores the
        // empty sprite), so only placeable types must have unique textures.
        if (isPlaceable(type))
            textures.insert(tileTypeToTextureName(type));
    }

    CHECK_EQ(names.size(), allTileTypes.size());
    CHECK_EQ(textures.size(), allTileTypes.size() - 1);

    // VOID must resolve to the empty sprite: there is no dedicated texture.
    CHECK_EQ(tileTypeToTextureName(TileType::VOID), tileTypeToTextureName(TileType::EMPTY));
}

ISO_TEST(random_writes_keep_the_grid_valid)
{
    // Deterministic fuzzing: thousands of writes over a small grid, then check
    // that the invariants still hold and that reads match what was written.
    iso::Rng rng(12345);
    TileGrid grid(7, 5, 2);

    for (int iteration = 0; iteration < 5000; ++iteration)
    {
        const int x = rng.range(-1, grid.width());
        const int y = rng.range(-1, grid.height());
        const int z = rng.range(-1, grid.layers());
        const auto type = static_cast<TileType>(rng.range(0, static_cast<int>(allTileTypes.size()) - 1));

        const bool changed = grid.set(x, y, z, type);

        if (!grid.isInBounds(x, y, z))
        {
            CHECK(!changed);
            continue;
        }

        const TileType stored = grid.at(x, y, z);
        CHECK(stored != TileType::VOID);
        CHECK_EQ(stored == (type == TileType::VOID ? TileType::EMPTY : type), true);
    }

    std::string reason;
    CHECK_NOTE(grid.validate(&reason), reason);
}

int main(int argc, char** argv)
{
    return testing::run(argc, argv);
}

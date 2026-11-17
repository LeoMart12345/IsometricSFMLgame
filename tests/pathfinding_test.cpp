// pathfinding_test.cpp
//
// A* is checked against an independent, deliberately naive Dijkstra
// implementation that follows the same movement rules. If the two disagree the
// path is not optimal, which is the kind of bug that is invisible in game but
// makes monsters walk in silly circles.
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

#include "core/rng.hpp"
#include "game/pathfinding.hpp"
#include "map/tile_grid.hpp"
#include "test_framework.hpp"

namespace
{
TileGrid makeOpenGrid(int width, int height)
{
    TileGrid grid(width, height, 1);
    grid.fill(TileType::GRASS);
    return grid;
}

void wall(TileGrid& grid, int x, int y)
{
    grid.set(x, y, 0, TileType::EMPTY); // no floor == not walkable
}

constexpr int kWall = -1;

// Reference implementation: plain Dijkstra over the same rules (8 directions
// when `allowDiagonal`, otherwise 4, no corner cutting, terrain move costs in
// half-cell units). Returns std::numeric_limits<int>::max() when the goal cannot
// be reached.
int referenceCost(const TileGrid& grid, iso::GridPoint start, iso::GridPoint goal, bool allowDiagonal = true)
{
    constexpr int kInfinity = std::numeric_limits<int>::max();

    const int width = grid.width();
    const int height = grid.height();
    if (!grid.isWalkable(goal.x, goal.y, 0))
        return kInfinity;

    std::vector<int> distance(static_cast<std::size_t>(width * height), kInfinity);
    std::vector<bool> settled(static_cast<std::size_t>(width * height), false);

    const auto index = [width](iso::GridPoint point) { return point.y * width + point.x; };
    distance[static_cast<std::size_t>(index(start))] = 0;

    constexpr int kOffsets[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (int iteration = 0; iteration < width * height; ++iteration)
    {
        int best = -1;
        for (int i = 0; i < width * height; ++i)
        {
            const auto cell = static_cast<std::size_t>(i);
            if (settled[cell] || distance[cell] == kInfinity)
                continue;
            if (best < 0 || distance[cell] < distance[static_cast<std::size_t>(best)])
                best = i;
        }

        if (best < 0)
            break;

        const iso::GridPoint current{best % width, best / width};
        settled[static_cast<std::size_t>(best)] = true;

        if (current.x == goal.x && current.y == goal.y)
            return distance[static_cast<std::size_t>(best)];

        for (const auto& offset : kOffsets)
        {
            const iso::GridPoint next{current.x + offset[0], current.y + offset[1]};
            if (!grid.isWalkable(next.x, next.y, 0))
                continue;

            const bool diagonal = offset[0] != 0 && offset[1] != 0;
            if (diagonal && !allowDiagonal)
                continue;

            if (diagonal && (!grid.isWalkable(current.x + offset[0], current.y, 0) ||
                             !grid.isWalkable(current.x, current.y + offset[1], 0)))
                continue;

            const int unit = diagonal ? game::diagonalStepUnits : game::orthogonalStepUnits;
            const int candidate = distance[static_cast<std::size_t>(best)] + grid.moveCost(next.x, next.y, 0) * unit;
            const auto nextIndex = static_cast<std::size_t>(index(next));
            if (candidate < distance[nextIndex])
                distance[nextIndex] = candidate;
        }
    }

    return kInfinity;
}

// A blocked goal is reached from a walkable neighbour, so the reference answer
// is the cheapest walkable cell next to it (4-way adjacency when diagonals are
// disabled, matching the search).
int referenceBlockedGoalCost(const TileGrid& grid, iso::GridPoint start, iso::GridPoint goal,
                             bool allowDiagonal = true)
{
    constexpr int kOffsets[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    constexpr int kInfinity = std::numeric_limits<int>::max();

    int best = kInfinity;
    for (const auto& offset : kOffsets)
    {
        if (!allowDiagonal && offset[0] != 0 && offset[1] != 0)
            continue;

        best = std::min(best, referenceCost(grid, start, {goal.x + offset[0], goal.y + offset[1]}, allowDiagonal));
    }

    return best;
}

// Every step must be walkable and adjacent to the previous position, and the
// first step must be reachable from the start in one move.
void checkPathIsWellFormed(const TileGrid& grid, iso::GridPoint start, const game::PathResult& path)
{
    CHECK(!path.steps.empty());

    iso::GridPoint previous = start;
    int accumulated = 0;

    for (const game::PathStep& step : path.steps)
    {
        CHECK_NOTE(game::areAdjacent(previous, step.cell), "consecutive path cells are not adjacent");
        CHECK_NOTE(grid.isWalkable(step.cell.x, step.cell.y, 0), "path crosses a non-walkable cell");

        const bool diagonal = std::abs(step.cell.x - previous.x) == 1 && std::abs(step.cell.y - previous.y) == 1;
        const int unit = diagonal ? game::diagonalStepUnits : game::orthogonalStepUnits;
        CHECK_EQ(step.cost, grid.moveCost(step.cell.x, step.cell.y, 0) * unit);

        // No corner cutting: both orthogonal neighbours of a diagonal step are open.
        if (diagonal)
        {
            CHECK(grid.isWalkable(previous.x, step.cell.y, 0));
            CHECK(grid.isWalkable(step.cell.x, previous.y, 0));
        }

        accumulated += step.cost;
        previous = step.cell;
    }

    CHECK_EQ(accumulated, path.totalCost);
}
} // namespace


ISO_TEST(straight_line_is_optimal)
{
    TileGrid grid = makeOpenGrid(10, 1);
    const game::PathResult path = game::findPath(grid, 0, {0, 0}, {9, 0});

    CHECK(path.reachedGoal);
    CHECK_EQ(path.steps.size(), static_cast<std::size_t>(9));
    CHECK_EQ(path.totalCost, 9 * game::orthogonalStepUnits);
    checkPathIsWellFormed(grid, {0, 0}, path);
    CHECK_EQ(path.totalCost, referenceCost(grid, {0, 0}, {9, 0}));
}

ISO_TEST(diagonals_are_used_when_cheaper)
{
    TileGrid grid = makeOpenGrid(5, 5);
    const game::PathResult path = game::findPath(grid, 0, {0, 0}, {4, 4});

    CHECK(path.reachedGoal);
    CHECK_EQ(path.steps.size(), static_cast<std::size_t>(4));
    CHECK_EQ(path.totalCost, 4 * game::diagonalStepUnits);
    checkPathIsWellFormed(grid, {0, 0}, path);
}

ISO_TEST(diagonals_can_be_disabled)
{
    TileGrid grid = makeOpenGrid(5, 5);

    game::PathOptions options;
    options.allowDiagonal = false;

    const game::PathResult path = game::findPath(grid, 0, {0, 0}, {4, 4}, options);

    CHECK(path.reachedGoal);
    CHECK_EQ(path.steps.size(), static_cast<std::size_t>(8));
    CHECK_EQ(path.totalCost, 8 * game::orthogonalStepUnits);
    checkPathIsWellFormed(grid, {0, 0}, path);
}

ISO_TEST(paths_route_around_walls)
{
    TileGrid grid = makeOpenGrid(9, 9);

    // A wall with a gap at the bottom, so a path exists but not in a straight line.
    for (int y = 0; y < 8; ++y)
        wall(grid, 4, y);

    const game::PathResult path = game::findPath(grid, 0, {0, 4}, {8, 4});

    CHECK(path.reachedGoal);
    checkPathIsWellFormed(grid, {0, 4}, path);
    CHECK_EQ(path.totalCost, referenceCost(grid, {0, 4}, {8, 4}));

    // The detour has to pass through the gap in the wall.
    bool usedGap = false;
    for (const game::PathStep& step : path.steps)
        usedGap = usedGap || (step.cell.x == 4 && step.cell.y == 8);
    CHECK(usedGap);
}

ISO_TEST(unreachable_goal_returns_an_empty_path)
{
    TileGrid grid = makeOpenGrid(9, 9);

    // Ring wall around the goal: no way in.
    for (int d = -1; d <= 1; ++d)
    {
        wall(grid, 5 + d, 4);
        wall(grid, 5 + d, 6);
        wall(grid, 4, 5 + d);
        wall(grid, 6, 5 + d);
    }

    const game::PathResult path = game::findPath(grid, 0, {0, 0}, {5, 5});

    CHECK(!path.found);
    CHECK(!path.reachedGoal);
    CHECK(path.empty());
    CHECK_EQ(path.totalCost, 0);
}

ISO_TEST(blocked_goal_walks_to_a_neighbour)
{
    TileGrid grid = makeOpenGrid(8, 8);
    wall(grid, 5, 5);

    game::PathOptions options;
    options.allowBlockedGoal = true;

    const game::PathResult path = game::findPath(grid, 0, {1, 1}, {5, 5}, options);

    CHECK(path.found);
    CHECK(!path.empty());
    CHECK(!path.reachedGoal); // stopped next to the blocked goal
    checkPathIsWellFormed(grid, {1, 1}, path);
    CHECK(game::areAdjacent(path.steps.back().cell, {5, 5}));
    CHECK_EQ(path.totalCost, referenceBlockedGoalCost(grid, {1, 1}, {5, 5}));
}

ISO_TEST(blocked_goal_can_be_rejected)
{
    TileGrid grid = makeOpenGrid(8, 8);
    wall(grid, 5, 5);

    game::PathOptions options;
    options.allowBlockedGoal = false;

    const game::PathResult path = game::findPath(grid, 0, {1, 1}, {5, 5}, options);
    CHECK(!path.found);
    CHECK(path.empty());
}

ISO_TEST(a_fully_blocked_goal_has_no_path)
{
    TileGrid grid = makeOpenGrid(8, 8);

    // Goal and all of its neighbours are walls.
    wall(grid, 5, 5);
    wall(grid, 4, 4);
    wall(grid, 5, 4);
    wall(grid, 6, 4);
    wall(grid, 4, 5);
    wall(grid, 6, 5);
    wall(grid, 4, 6);
    wall(grid, 5, 6);
    wall(grid, 6, 6);

    const game::PathResult path = game::findPath(grid, 0, {0, 0}, {5, 5});
    CHECK(path.empty());
}

ISO_TEST(corners_are_never_cut)
{
    TileGrid grid = makeOpenGrid(3, 3);

    // Both orthogonal cells between (0,0) and (1,1) are walls, so a diagonal
    // step would squeeze between them and must not be allowed.
    wall(grid, 1, 0);
    wall(grid, 0, 1);

    const game::PathResult path = game::findPath(grid, 0, {0, 0}, {1, 1});
    CHECK(path.empty());
}

ISO_TEST(terrain_cost_is_part_of_the_path)
{
    TileGrid grid = makeOpenGrid(3, 1);
    grid.set(1, 0, 0, TileType::FOREST);

    game::PathOptions options;
    options.allowDiagonal = false;

    const game::PathResult path = game::findPath(grid, 0, {0, 0}, {2, 0}, options);

    CHECK(path.reachedGoal);
    CHECK_EQ(path.steps.size(), static_cast<std::size_t>(2));
    CHECK_EQ(path.steps[0].cost, game::orthogonalStepUnits * moveCost(TileType::FOREST));
    CHECK_EQ(path.totalCost,
             game::orthogonalStepUnits * (moveCost(TileType::FOREST) + moveCost(TileType::GRASS)));
}

ISO_TEST(forest_is_avoided_when_a_detour_is_cheaper)
{
    TileGrid grid = makeOpenGrid(6, 3);

    // Three woods cells block the straight route; going around them is cheaper
    // (12 vs 16 half-cells), so the path must not touch them.
    for (int x = 2; x <= 4; ++x)
        grid.set(x, 1, 0, TileType::FOREST);

    const game::PathResult path = game::findPath(grid, 0, {0, 1}, {5, 1});

    CHECK(path.reachedGoal);
    for (const game::PathStep& step : path.steps)
        CHECK(!(step.cell.y == 1 && step.cell.x >= 2 && step.cell.x <= 4));
    CHECK_EQ(path.totalCost, referenceCost(grid, {0, 1}, {5, 1}));
    CHECK_NOTE(path.totalCost < 16, "the path did not avoid the expensive terrain");
}

ISO_TEST(start_next_to_a_blocked_goal_needs_no_steps)
{
    TileGrid grid = makeOpenGrid(8, 8);
    wall(grid, 5, 5);

    // Diagonal, orthogonal and "the target is right next to me" cases: all of
    // them are a success that needs no movement, not a crash and not an empty
    // failure (the fallback used to assert here).
    const iso::GridPoint starts[] = {{4, 4}, {4, 5}, {5, 4}, {6, 6}};
    for (const iso::GridPoint start : starts)
    {
        const game::PathResult path = game::findPath(grid, 0, start, {5, 5});

        CHECK_NOTE(path.found, "an adjacent blocked goal reported no path");
        CHECK(path.empty());
        CHECK(!path.reachedGoal); // the goal itself is still occupied
        CHECK_EQ(path.totalCost, 0);
    }
}

ISO_TEST(blocked_goals_produce_optimal_paths_on_random_maps)
{
    // Same idea as random_maps_always_produce_optimal_paths, but the goal is a
    // wall: A* must stop on the cheapest neighbour. The old heuristic measured
    // to the blocked cell itself, overestimated by one step and regularly picked
    // a more expensive neighbour (see the one-step diagonal/orthogonal cases).
    iso::Rng rng(90210);

    for (int round = 0; round < 120; ++round)
    {
        const int width = rng.range(3, 10);
        const int height = rng.range(3, 10);
        TileGrid grid = makeOpenGrid(width, height);

        const int obstacleCount = rng.range(0, (width * height) / 3);
        for (int i = 0; i < obstacleCount; ++i)
        {
            const int x = rng.range(0, width - 1);
            const int y = rng.range(0, height - 1);
            grid.set(x, y, 0, rng.chance(0.5f) ? TileType::EMPTY : TileType::FOREST);
        }

        const iso::GridPoint start{rng.range(0, width - 1), rng.range(0, height - 1)};
        const iso::GridPoint goal{rng.range(0, width - 1), rng.range(0, height - 1)};

        if (start.x == goal.x && start.y == goal.y)
            continue;

        grid.set(goal.x, goal.y, 0, TileType::EMPTY); // block the goal itself

        const int expected = referenceBlockedGoalCost(grid, start, goal);
        const game::PathResult path = game::findPath(grid, 0, start, goal); // allowBlockedGoal is on

        if (expected == std::numeric_limits<int>::max())
        {
            CHECK_NOTE(!path.found, "found a path where the reference found none");
            continue;
        }

        CHECK_NOTE(path.found, "the reference found a path but A* did not");
        CHECK_EQ(path.totalCost, expected);
        CHECK(!path.reachedGoal);

        if (!path.empty())
        {
            CHECK(game::areAdjacent(path.steps.back().cell, goal));
            checkPathIsWellFormed(grid, start, path);
        }
    }
}

ISO_TEST(an_actor_on_a_wall_can_still_walk_out)
{
    // Walls can end up under an actor (map editor, loaded save), so the start
    // cell is allowed to be non-walkable: the search steps out of it instead of
    // reporting "no path".
    TileGrid grid = makeOpenGrid(6, 6);
    wall(grid, 2, 2);

    const game::PathResult path = game::findPath(grid, 0, {2, 2}, {5, 5});

    CHECK(path.found);
    CHECK(path.reachedGoal);
    checkPathIsWellFormed(grid, {2, 2}, path);
}

ISO_TEST(start_equals_goal_is_a_trivial_success)
{
    TileGrid grid = makeOpenGrid(4, 4);
    const game::PathResult path = game::findPath(grid, 0, {2, 2}, {2, 2});

    CHECK(path.found);
    CHECK(path.reachedGoal);
    CHECK(path.empty());
    CHECK_EQ(path.totalCost, 0);
}

ISO_TEST(out_of_bounds_requests_are_rejected)
{
    TileGrid grid = makeOpenGrid(4, 4);

    CHECK(game::findPath(grid, 0, {-1, 0}, {2, 2}).empty());
    CHECK(game::findPath(grid, 0, {0, 0}, {4, 2}).empty());
    CHECK(game::findPath(grid, 5, {0, 0}, {2, 2}).empty()); // layer does not exist

    game::PathOptions noBudget;
    noBudget.maxExpandedNodes = 0;
    CHECK(game::findPath(grid, 0, {0, 0}, {2, 2}, noBudget).empty());
}

ISO_TEST(search_is_deterministic)
{
    TileGrid grid = makeOpenGrid(12, 12);
    for (int y = 2; y < 10; ++y)
        wall(grid, 6, y);

    const game::PathResult first = game::findPath(grid, 0, {1, 1}, {10, 9});
    const game::PathResult second = game::findPath(grid, 0, {1, 1}, {10, 9});

    CHECK(first.reachedGoal);
    CHECK_EQ(first.steps.size(), second.steps.size());
    CHECK_EQ(first.totalCost, second.totalCost);

    for (std::size_t i = 0; i < first.steps.size() && i < second.steps.size(); ++i)
        CHECK(first.steps[i].cell.x == second.steps[i].cell.x &&
              first.steps[i].cell.y == second.steps[i].cell.y);
}

ISO_TEST(node_budget_stops_the_search)
{
    TileGrid grid = makeOpenGrid(40, 40);

    game::PathOptions options;
    options.maxExpandedNodes = 5;

    const game::PathResult path = game::findPath(grid, 0, {0, 0}, {39, 39}, options);
    CHECK(path.empty());
}

ISO_TEST(random_maps_always_produce_optimal_paths)
{
    // Deterministic fuzzing: random obstacle fields and endpoints, compared
    // against the reference Dijkstra above.
    iso::Rng rng(2024);

    for (int round = 0; round < 60; ++round)
    {
        const int width = rng.range(3, 14);
        const int height = rng.range(3, 14);
        TileGrid grid = makeOpenGrid(width, height);

        const int obstacleCount = rng.range(0, (width * height) / 3);
        for (int i = 0; i < obstacleCount; ++i)
        {
            const int x = rng.range(0, width - 1);
            const int y = rng.range(0, height - 1);
            grid.set(x, y, 0, rng.chance(0.5f) ? TileType::EMPTY : TileType::FOREST);
        }

        const iso::GridPoint start{rng.range(0, width - 1), rng.range(0, height - 1)};
        const iso::GridPoint goal{rng.range(0, width - 1), rng.range(0, height - 1)};

        game::PathOptions options;
        options.allowBlockedGoal = false; // compare like-for-like with the reference

        const game::PathResult path = game::findPath(grid, 0, start, goal, options);
        const int expected = referenceCost(grid, start, goal);

        if (expected == std::numeric_limits<int>::max())
        {
            CHECK_NOTE(path.empty(), "found a path where the reference found none");
            continue;
        }

        CHECK_NOTE(path.reachedGoal, "the reference found a path but A* did not");
        if (!path.empty())
        {
            CHECK_EQ(path.totalCost, expected);
            checkPathIsWellFormed(grid, start, path);
        }
    }
}

ISO_TEST(orthogonal_only_searches_stay_optimal)
{
    // The heuristic changes shape when diagonals are disabled, and every other
    // randomized comparison here lets diagonals through. Fuzz the 4-way branch as
    // well, for open goals and for blocked ones (the fallback neighbour scan).
    iso::Rng rng(4711);

    for (int round = 0; round < 80; ++round)
    {
        const int width = rng.range(3, 11);
        const int height = rng.range(3, 11);
        TileGrid grid = makeOpenGrid(width, height);

        const int obstacleCount = rng.range(0, (width * height) / 3);
        for (int i = 0; i < obstacleCount; ++i)
        {
            const int x = rng.range(0, width - 1);
            const int y = rng.range(0, height - 1);
            grid.set(x, y, 0, rng.chance(0.5f) ? TileType::EMPTY : TileType::FOREST);
        }

        const iso::GridPoint start{rng.range(0, width - 1), rng.range(0, height - 1)};
        const iso::GridPoint goal{rng.range(0, width - 1), rng.range(0, height - 1)};

        game::PathOptions options;
        options.allowDiagonal = false;

        // 1. The goal itself is walkable: the path must reach it, orthogonally only.
        const int expected = referenceCost(grid, start, goal, /*allowDiagonal=*/false);
        if (expected != std::numeric_limits<int>::max())
        {
            game::PathOptions strict = options;
            strict.allowBlockedGoal = false;
            const game::PathResult path = game::findPath(grid, 0, start, goal, strict);

            CHECK_NOTE(path.reachedGoal, "the reference found a path but A* did not");
            CHECK_EQ(path.totalCost, expected);

            if (!path.empty())
            {
                checkPathIsWellFormed(grid, start, path);
                iso::GridPoint previous = start;
                for (const game::PathStep& step : path.steps)
                {
                    const bool diagonal = std::abs(step.cell.x - previous.x) == 1 &&
                                          std::abs(step.cell.y - previous.y) == 1;
                    CHECK_NOTE(!diagonal, "diagonal step in a 4-way search");
                    previous = step.cell;
                }
            }
        }

        // 2. The goal is a wall: A* must stop on the cheapest orthogonal neighbour.
        if (start.x == goal.x && start.y == goal.y)
            continue;

        grid.set(goal.x, goal.y, 0, TileType::EMPTY);

        const int blockedExpected = referenceBlockedGoalCost(grid, start, goal, /*allowDiagonal=*/false);
        const game::PathResult blocked = game::findPath(grid, 0, start, goal, options); // allowBlockedGoal

        if (blockedExpected == std::numeric_limits<int>::max())
        {
            CHECK_NOTE(!blocked.found, "found a path where the reference found none");
            continue;
        }

        CHECK_NOTE(blocked.found, "the reference found a path but A* did not");
        CHECK_EQ(blocked.totalCost, blockedExpected);
        CHECK(!blocked.reachedGoal);

        if (!blocked.empty())
        {
            CHECK(game::areAdjacent(blocked.steps.back().cell, goal));
            checkPathIsWellFormed(grid, start, blocked);
        }
    }
}

int main(int argc, char** argv)
{
    return testing::run(argc, argv);
}

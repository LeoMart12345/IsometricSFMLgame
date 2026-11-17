// pathfinding.hpp
#pragma once

#include <vector>

#include "map/grid_geometry.hpp"
#include "map/tile_grid.hpp"

// Pathfinding over one layer of a TileGrid.
//
// Used by the player (click to move) and by the monster AI, so it must be:
//   * deterministic    - same grid/start/goal always produces the same path
//                        (ties are broken by a fixed rule, never by pointer
//                        order or container iteration order),
//   * rule driven      - walkability and move costs come from
//                        tile_properties.hpp, never from hardcoded checks,
//   * bounded          - a node budget stops a pathological level from stalling
//                        a frame.
namespace game
{
// Movement costs are integers in half-cell units, which keeps the search exact
// (no float comparisons) while still making a diagonal step cost 1.5 cells.
inline constexpr int orthogonalStepUnits = 2;
inline constexpr int diagonalStepUnits = 3;

struct PathOptions
{
    // Safety valve: the search gives up after expanding this many cells.
    int maxExpandedNodes = 8192;

    bool allowDiagonal = true;

    // When the clicked/goal cell is not walkable (a wall, water, a monster), path
    // to the closest reachable neighbour instead of failing. This is what makes
    // "click on the enemy to walk into range" work.
    bool allowBlockedGoal = true;
};

struct PathStep
{
    iso::GridPoint cell{};
    int cost = 0; // half-cell units (see orthogonalStepUnits)
};

struct PathResult
{
    // Does not include `start`; the last step is the goal (or the closest
    // reachable neighbour of a blocked goal). Empty when the actor already
    // stands on the goal or next to a blocked goal; `found` tells that apart
    // from "no path exists".
    std::vector<PathStep> steps;
    bool reachedGoal = false;
    int totalCost = 0;

    // True when a path exists. Not the same as !empty(): no movement is needed
    // when the actor is already on the goal, or already next to a blocked goal.
    bool found = false;

    [[nodiscard]] bool empty() const { return steps.empty(); }
};

// Chebyshev distance: 1 for any of the 8 neighbours, which is what "adjacent"
// means for melee.
[[nodiscard]] int gridDistance(iso::GridPoint first, iso::GridPoint second);

[[nodiscard]] bool areAdjacent(iso::GridPoint first, iso::GridPoint second);

// A* search on layer `z`. Returns a result with `found == false` when the goal
// is unreachable, when a position is outside the grid, or when the node budget
// is exhausted. A successful search can still have no steps: the actor may
// already stand on the goal, or already next to a blocked goal.
//
// `start` does not have to be walkable (a wall may have been placed under the
// actor); the search simply steps out of it.
[[nodiscard]] PathResult findPath(const TileGrid& grid, int z, iso::GridPoint start, iso::GridPoint goal,
                                  const PathOptions& options = {});
} // namespace game

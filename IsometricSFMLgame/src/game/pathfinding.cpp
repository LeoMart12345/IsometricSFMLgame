// pathfinding.cpp
#include "game/pathfinding.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <vector>

#include "core/assert.hpp"
#include "map/tile_properties.hpp"

namespace
{
constexpr int kUnreachable = std::numeric_limits<int>::max();

constexpr int kDirections[8][2] = {
    {1, 0},  {-1, 0}, {0, 1}, {0, -1}, // orthogonal first, so ties prefer straight lines
    {1, 1},  {1, -1}, {-1, 1}, {-1, -1},
};

struct Node
{
    int costSoFar = 0; // g
    int estimate = 0;  // h
    int index = 0;     // local index inside the layer

    [[nodiscard]] int total() const { return costSoFar + estimate; }
};

// std::priority_queue is a max-heap, so this comparator turns it into a min-heap
// ordered by f, then h, then index. Ordering by index last makes the search
// deterministic when several cells are equally good.
struct NodeGreater
{
    bool operator()(const Node& lhs, const Node& rhs) const
    {
        if (lhs.total() != rhs.total())
            return lhs.total() > rhs.total();
        if (lhs.estimate != rhs.estimate)
            return lhs.estimate > rhs.estimate;
        return lhs.index > rhs.index;
    }
};

using NodeQueue = std::priority_queue<Node, std::vector<Node>, NodeGreater>;
} // namespace

int game::gridDistance(iso::GridPoint first, iso::GridPoint second)
{
    // Subtract in 64 bit: `first.x - second.x` itself can overflow for two valid
    // int coordinates (e.g. INT_MIN and INT_MAX), which would make std::abs()
    // undefined and could report two distant cells as adjacent.
    const std::int64_t dx = static_cast<std::int64_t>(first.x) - static_cast<std::int64_t>(second.x);
    const std::int64_t dy = static_cast<std::int64_t>(first.y) - static_cast<std::int64_t>(second.y);
    const std::uint64_t distance = std::max(static_cast<std::uint64_t>(dx < 0 ? -dx : dx),
                                            static_cast<std::uint64_t>(dy < 0 ? -dy : dy));

    // The API returns int; saturate rather than truncate an unrepresentable
    // distance. Every real grid is far below this limit.
    constexpr std::uint64_t kMaxDistance = static_cast<std::uint64_t>(std::numeric_limits<int>::max());
    return distance >= kMaxDistance ? std::numeric_limits<int>::max() : static_cast<int>(distance);
}

bool game::areAdjacent(iso::GridPoint first, iso::GridPoint second)
{
    // Exact 64 bit comparison: adjacency only cares about differences of one,
    // and this cannot overflow next to INT_MIN/INT_MAX.
    const std::int64_t dx = static_cast<std::int64_t>(first.x) - static_cast<std::int64_t>(second.x);
    const std::int64_t dy = static_cast<std::int64_t>(first.y) - static_cast<std::int64_t>(second.y);
    return dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1 && (dx != 0 || dy != 0);
}

game::PathResult game::findPath(const TileGrid& grid, int z, iso::GridPoint start, iso::GridPoint goal,
                                const PathOptions& options)
{
    PathResult result;

    // Invalid requests are answered, not asserted: they come from mouse input.
    if (!grid.isInBounds(start.x, start.y, z) || !grid.isInBounds(goal.x, goal.y, z))
        return result;

    if (options.maxExpandedNodes <= 0)
        return result;

    if (start.x == goal.x && start.y == goal.y)
    {
        result.reachedGoal = true;
        result.found = true;
        return result;
    }

    const int width = grid.width();
    const auto localIndex = [width](iso::GridPoint point) { return point.y * width + point.x; };

    // When the goal itself cannot be entered, accept any walkable neighbour.
    const bool goalIsGoal = grid.isWalkable(goal.x, goal.y, z);
    if (!goalIsGoal && !options.allowBlockedGoal)
        return result;

    std::vector<char> isGoalCell;
    if (!goalIsGoal)
    {
        isGoalCell.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(grid.height()), 0);

        const int directionCount = options.allowDiagonal ? 8 : 4;
        int goalCount = 0;
        for (int direction = 0; direction < directionCount; ++direction)
        {
            const int x = goal.x + kDirections[direction][0];
            const int y = goal.y + kDirections[direction][1];
            if (!grid.isWalkable(x, y, z))
                continue;

            isGoalCell[static_cast<std::size_t>(localIndex({x, y}))] = 1;
            ++goalCount;
        }

        if (goalCount == 0)
            return result;
    }

    const auto isGoal = [&](iso::GridPoint point) {
        if (goalIsGoal)
            return point.x == goal.x && point.y == goal.y;

        return isGoalCell[static_cast<std::size_t>(localIndex(point))] != 0;
    };

    // Already standing on one of the cells that count as "arrived": the search
    // would pop the start and then reconstruct an empty parent chain, so answer
    // directly instead. No movement is needed, but the request did succeed.
    if (!goalIsGoal && isGoal(start))
    {
        result.found = true;
        return result;
    }

    // Octile heuristic: exact when every step costs one cell, admissible
    // otherwise (terrain can only be slower, never faster).
    const auto heuristic = [&](iso::GridPoint point) {
        const int dx = std::abs(point.x - goal.x);
        const int dy = std::abs(point.y - goal.y);

        if (!options.allowDiagonal)
        {
            const int distance = orthogonalStepUnits * (dx + dy);

            // A blocked goal is approached from a neighbour, so the last step is
            // not part of the estimate. Measuring to the blocked cell itself
            // would overestimate, which breaks A*'s optimality and can make it
            // walk to the expensive side of the goal.
            return goalIsGoal ? distance : std::max(0, distance - orthogonalStepUnits);
        }

        const int diagonal = std::min(dx, dy);
        const int straight = std::max(dx, dy) - diagonal;
        const int distance = diagonalStepUnits * diagonal + orthogonalStepUnits * straight;

        return goalIsGoal ? distance : std::max(0, distance - diagonalStepUnits);
    };

    const auto cellCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(grid.height());
    std::vector<int> costSoFar(cellCount, kUnreachable);
    std::vector<int> parent(cellCount, -1);

    NodeQueue open;
    const int startIndex = localIndex(start);
    costSoFar[static_cast<std::size_t>(startIndex)] = 0;
    open.push({0, heuristic(start), startIndex});

    int expanded = 0;
    int goalIndex = -1;
    const int directionCount = options.allowDiagonal ? 8 : 4;

    while (!open.empty())
    {
        // Copied, because the reference returned by top() is invalidated by push().
        const Node current = open.top();
        open.pop();

        const auto currentIndex = static_cast<std::size_t>(current.index);
        if (current.costSoFar > costSoFar[currentIndex])
            continue; // stale entry: a cheaper route to this cell was found later

        const iso::GridPoint currentPoint{current.index % width, current.index / width};

        if (isGoal(currentPoint))
        {
            goalIndex = current.index;
            break;
        }

        // The budget counts cells whose neighbours are expanded. Checking before
        // the increment makes the limit exact: maxExpandedNodes == 1 expands only
        // the start cell instead of also expanding the first neighbour.
        if (expanded >= options.maxExpandedNodes)
            break;
        ++expanded;

        for (int direction = 0; direction < directionCount; ++direction)
        {
            const int dx = kDirections[direction][0];
            const int dy = kDirections[direction][1];
            const iso::GridPoint next{currentPoint.x + dx, currentPoint.y + dy};

            if (!grid.isWalkable(next.x, next.y, z))
                continue;

            const bool diagonal = dx != 0 && dy != 0;
            if (diagonal)
            {
                // No corner cutting: both orthogonal cells must be open, otherwise
                // an actor could squeeze diagonally between two walls.
                if (!grid.isWalkable(currentPoint.x + dx, currentPoint.y, z) ||
                    !grid.isWalkable(currentPoint.x, currentPoint.y + dy, z))
                    continue;
            }

            const int unitCost = diagonal ? diagonalStepUnits : orthogonalStepUnits;
            const int candidate = current.costSoFar + grid.moveCost(next.x, next.y, z) * unitCost;

            const auto nextIndex = static_cast<std::size_t>(localIndex(next));
            if (candidate >= costSoFar[nextIndex])
                continue;

            costSoFar[nextIndex] = candidate;
            parent[nextIndex] = current.index;
            open.push({candidate, heuristic(next), static_cast<int>(nextIndex)});
        }
    }

    if (goalIndex < 0)
        return result;

    // Walk the parent chain back to the start, then reverse it.
    std::vector<PathStep> reversed;
    int walk = goalIndex;
    while (walk != startIndex && walk >= 0)
    {
        const auto walkIndex = static_cast<std::size_t>(walk);
        const int previous = parent[walkIndex];
        const int stepCost =
            (previous >= 0) ? costSoFar[walkIndex] - costSoFar[static_cast<std::size_t>(previous)] : 0;

        reversed.push_back({{walk % width, walk / width}, stepCost});
        walk = previous;
    }

    if (walk != startIndex)
        return result; // broken chain: report "unreachable" instead of garbage

    result.steps.assign(reversed.rbegin(), reversed.rend());

    // "Reached the goal" means standing *on* the goal. When the goal itself is
    // blocked we stop on the closest neighbour, which is a different outcome for
    // the caller (e.g. "walked next to the monster" is not "arrived").
    result.reachedGoal = goalIsGoal;
    result.totalCost = costSoFar[static_cast<std::size_t>(goalIndex)];
    result.found = true;

    ISO_ASSERT(!result.steps.empty());
    return result;
}

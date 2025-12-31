#pragma once
#include <vector>
#include "PathNode.h"
#include "Types.h"

// Forward declaration
class Map;

// ============================================================
// Pathfinder.h
// Implements the A* pathfinding algorithm with optional
// safety-aware routing using a danger grid.
// ============================================================
class Pathfinder {
public:
    // Runs A* search on the given map.
    // If 'dangerGrid' is provided, safer routes (lower danger) are preferred.
    // Returns true and fills 'outPath' with cells from start (excluded) to goal (included)
    // if a path exists; otherwise returns false.
    static bool AStar(
        const Map& world,
        const Vec2i& start,
        const Vec2i& goal,
        std::vector<Vec2i>& outPath,
        const int dangerGrid[MSZ][MSZ] = nullptr
    );
};

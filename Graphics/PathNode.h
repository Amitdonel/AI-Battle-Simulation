#pragma once
#include "Definitions.h"
#include "Types.h"

// ============================================================
// PathNode.h
// Defines A* pathfinding node and comparison logic for the queue
// ============================================================

struct PathNode {
    Vec2i p;               // position on map (row, col)
    int g = 0;             // cost from start
    int f = 0;             // total cost (g + h)
    Vec2i parent{ -1, -1 };// parent node coordinates

    PathNode() = default;
    PathNode(int r, int c, int gCost, int fCost, Vec2i pr)
        : p{ r, c }, g(gCost), f(fCost), parent(pr) {
    }
};

// Comparator for priority queue (min-heap by f)
struct ComparePathNode {
    bool operator()(const PathNode& a, const PathNode& b) const {
        return a.f > b.f;
    }
};

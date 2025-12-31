#include "Pathfinder.h"
#include "Map.h"
#include "Definitions.h"
#include <queue>
#include <cstring>
#include <cmath>
#include <algorithm> // for std::reverse

// ============================================================
// Utility helpers
// ============================================================

// Clamp for older C++ versions
template <typename T>
inline T clampValue(T v, T lo, T hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Manhattan distance heuristic
static inline int manh(const Vec2i& a, const Vec2i& b) {
    return std::abs(a.r - b.r) + std::abs(a.c - b.c);
}

// ============================================================
// A* Implementation
// ============================================================
bool Pathfinder::AStar(
    const Map& world,
    const Vec2i& start,
    const Vec2i& goal,
    std::vector<Vec2i>& outPath,
    const int dangerGrid[MSZ][MSZ]
) {
    outPath.clear();
    if (start.r == goal.r && start.c == goal.c)
        return true;

    // Initialize arrays
    static int  gScore[MSZ][MSZ];
    static bool closed[MSZ][MSZ];
    static Vec2i parent[MSZ][MSZ];

    for (int i = 0; i < MSZ; ++i) {
        for (int j = 0; j < MSZ; ++j) {
            gScore[i][j] = 1e9;
            closed[i][j] = false;
            parent[i][j] = { -1, -1 };
        }
    }

    // Check walkable tiles
    auto passable = [&](int r, int c) -> bool {
        if (!world.inBounds(r, c)) return false;
        CellType ct = world.at(r, c);
        return (ct != ROCK && ct != WATER); // TREE is passable
        };

    std::priority_queue<PathNode, std::vector<PathNode>, ComparePathNode> open;
    gScore[start.r][start.c] = 0;
    open.push(PathNode(start.r, start.c, 0, manh(start, goal), { -1, -1 }));

    static const int dr[4] = { -1, 1, 0, 0 };
    static const int dc[4] = { 0, 0, -1, 1 };

    while (!open.empty()) {
        PathNode cur = open.top();
        open.pop();

        if (closed[cur.p.r][cur.p.c]) continue;
        closed[cur.p.r][cur.p.c] = true;

        // Goal reached: reconstruct path
        if (cur.p.r == goal.r && cur.p.c == goal.c) {
            Vec2i v = goal;
            while (!(v.r == start.r && v.c == start.c)) {
                outPath.push_back(v);
                Vec2i pr = parent[v.r][v.c];
                if (pr.r == -1) break;
                v = pr;
            }
            std::reverse(outPath.begin(), outPath.end());
            return true;
        }

        // Explore neighbors
        for (int k = 0; k < 4; ++k) {
            int nr = cur.p.r + dr[k];
            int nc = cur.p.c + dc[k];
            if (!passable(nr, nc)) continue;

            int baseCost = 1;

            // Add danger cost if grid is available
            if (dangerGrid != nullptr) {
                int dangerValue = dangerGrid[nr][nc];
                int dangerCost = clampValue(dangerValue / 10, 0, 10);
                baseCost += dangerCost;
            }

            int tentative = cur.g + baseCost;
            if (tentative < gScore[nr][nc]) {
                gScore[nr][nc] = tentative;
                parent[nr][nc] = cur.p;
                int f = tentative + manh({ nr, nc }, goal);
                open.push(PathNode(nr, nc, tentative, f, cur.p));
            }
        }
    }

    // No path found
    return false;
}

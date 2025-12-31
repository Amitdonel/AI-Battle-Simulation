#include "SafetyMap.h"
#include "Agent.h"
#include "Map.h"
#include <cmath>
#include <algorithm>

// ============================================================
// Constructor - initialize danger grid
// ============================================================
SafetyMap::SafetyMap() {
    for (int i = 0; i < MSZ; ++i)
        for (int j = 0; j < MSZ; ++j)
            grid[i][j] = 0;
}

// ============================================================
// Compute danger values from all visible enemy agents
// ============================================================
void SafetyMap::compute(const std::vector<Agent*>& enemies) {
    // Reset grid
    for (int r = 0; r < MSZ; ++r)
        for (int c = 0; c < MSZ; ++c)
            grid[r][c] = 0;

    // Add cumulative danger influence from all enemies
    for (auto* e : enemies) {
        if (!e->isAlive()) continue;
        int er = e->row();
        int ec = e->col();

        for (int r = 0; r < MSZ; ++r)
            for (int c = 0; c < MSZ; ++c) {
                int dist = std::abs(er - r) + std::abs(ec - c);
                if (dist < 1) dist = 1;

                // Normalized danger scale (0–20)
                int danger = std::max(0, 20 - dist * 2);
                grid[r][c] = std::min(20, grid[r][c] + danger);
            }
    }
}

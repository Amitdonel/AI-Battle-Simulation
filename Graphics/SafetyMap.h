#pragma once
#include "Definitions.h"
#include "Agent.h"
#include <vector>

// ============================================================
// SafetyMap
// Computes and stores a "danger value" for each cell on the map
// based on the visibility and proximity of enemy agents.
// ============================================================
class SafetyMap {
public:
    SafetyMap();

    // --- Main computation ---
    // Updates the danger grid based on enemy positions and visibility.
    void compute(const std::vector<Agent*>& enemies);

    // --- Accessors ---
    int get(int r, int c) const { return grid[r][c]; }

    // Safe accessor (normalized to [0,1]) for commander logic
    double at(int r, int c) const {
        if (r < 0 || r >= MSZ || c < 0 || c >= MSZ) return 1.0;
        return static_cast<double>(grid[r][c]) / 100.0;
    }

    // Full grid access (for debugging or weighting)
    const int(&getGrid() const)[MSZ][MSZ]{ return grid; }

        // Pointer accessor (used by Pathfinder and MoveToTarget)
    const int (*getGridPtr() const)[MSZ] { return grid; }

private:
    int grid[MSZ][MSZ]; // danger value per cell (0–20 typical range)
};

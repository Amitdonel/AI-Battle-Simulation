#pragma once
#include "Types.h"
#include "Definitions.h"

// ============================================================
// Map.h
// Defines the environment grid and helper logic for rendering,
// pathfinding, and line-of-sight calculations.
// ============================================================

// ----- Movement, vision, and fire blocking rules -----
inline bool BlocksMovement(CellType t) { return (t == ROCK || t == WATER); }
inline bool BlocksVision(CellType t) { return (t == ROCK || t == TREE); }
inline bool BlocksFire(CellType t) { return (t == ROCK || t == TREE); }

// ------------------------------------------------------------
// Map class
// ------------------------------------------------------------
class Map {
public:
    Map();

    // --- Core operations ---
    void initStructured();   // generate a structured environment (clusters + warehouses)
    void draw() const;       // render the entire map
    bool inBounds(int r, int c) const;

    // --- Accessors ---
    CellType at(int r, int c) const { return (CellType)grid[r][c]; }
    void set(int r, int c, CellType t) { grid[r][c] = (int)t; }

    // --- Visibility ---
    bool hasLineOfSight(const Vec2i& a, const Vec2i& b) const; // true if no blocking tiles between a and b

private:
    int grid[MSZ][MSZ];

    // Internal drawing helper
    void drawCell(int r, int c) const;
};

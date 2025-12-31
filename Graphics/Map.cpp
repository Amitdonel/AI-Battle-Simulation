#include "Map.h"
#include "glut.h"
#include <cstdlib>
#include <algorithm>

// ============================================================
// Constructor
// ============================================================
Map::Map() {
    for (int i = 0; i < MSZ; ++i)
        for (int j = 0; j < MSZ; ++j)
            grid[i][j] = EMPTY;
}

// ============================================================
// Boundaries
// ============================================================
bool Map::inBounds(int r, int c) const {
    return r >= 0 && r < MSZ && c >= 0 && c < MSZ;
}

// ============================================================
// Helper functions (local scope only)
// ============================================================

// Place a single tile if valid
static inline void stamp1(Map& m, int r, int c, CellType t) {
    if (m.inBounds(r, c)) m.set(r, c, t);
}

// Create a small connected cluster of N cells (3–5)
static void stampBlob(Map& m, int sr, int sc, int n, CellType t) {
    int r = sr, c = sc;
    for (int k = 0; k < n; ++k) {
        if (m.inBounds(r, c)) m.set(r, c, t);
        int dir = rand() % 4;
        if (dir == 0) r++;
        else if (dir == 1) r--;
        else if (dir == 2) c++;
        else c--;

        // ✅ manual clamp for full C++ compatibility
        if (r < 1) r = 1;
        else if (r > MSZ - 2) r = MSZ - 2;

        if (c < 1) c = 1;
        else if (c > MSZ - 2) c = MSZ - 2;
    }
}

// ============================================================
// Map initialization
// ============================================================
void Map::initStructured() {
    // Clear map
    for (int i = 0; i < MSZ; ++i)
        for (int j = 0; j < MSZ; ++j)
            grid[i][j] = EMPTY;

    // --- WATER clusters (light blue) ---
    for (int i = 0; i < 7; ++i) {
        int r = 8 + rand() % (MSZ - 16);
        int c = 8 + rand() % (MSZ - 16);
        int n = 3 + rand() % 3; // 3–5 cells
        stampBlob(*this, r, c, n, WATER);
    }

    // --- ROCK clusters (dark gray/brown) ---
    for (int i = 0; i < 7; ++i) {
        int r = 8 + rand() % (MSZ - 16);
        int c = 8 + rand() % (MSZ - 16);
        int n = 3 + rand() % 3;
        stampBlob(*this, r, c, n, ROCK);
    }

    // --- TREES (scattered, green triangles) ---
    int numTrees = (MSZ * MSZ) / 80;
    for (int i = 0; i < numTrees; ++i) {
        int r = 6 + rand() % (MSZ - 12);
        int c = 6 + rand() % (MSZ - 12);
        if (grid[r][c] == EMPTY)
            grid[r][c] = TREE;
    }

    // --- WAREHOUSES (ammo + med for each team) ---
    // Orange team (bottom-left)
    set(6, 6, SUPPLY_AMMO);
    set(6, 9, SUPPLY_MED);

    // Blue team (top-right)
    set(MSZ - 7, MSZ - 7, SUPPLY_AMMO);
    set(MSZ - 7, MSZ - 10, SUPPLY_MED);
}

// ============================================================
// Rendering
// ============================================================
void Map::drawCell(int r, int c) const {
    // Base terrain (grass)
    glColor3d(0.82, 0.95, 0.82);
    glBegin(GL_POLYGON);
    glVertex2d(c, r);
    glVertex2d(c, r + 1);
    glVertex2d(c + 1, r + 1);
    glVertex2d(c + 1, r);
    glEnd();

    const double inset = 0.10;
    double x0 = c + inset, y0 = r + inset;
    double x1 = c + 1 - inset, y1 = r + 1 - inset;

    switch ((CellType)grid[r][c]) {
    case EMPTY:
        break;

    case ROCK: { // dark rock
        glColor3d(0.35, 0.28, 0.20);
        glBegin(GL_POLYGON);
        glVertex2d(x0, y0); glVertex2d(x0, y1);
        glVertex2d(x1, y1); glVertex2d(x1, y0);
        glEnd();
        glColor3d(0, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2d(x0, y0); glVertex2d(x0, y1);
        glVertex2d(x1, y1); glVertex2d(x1, y0);
        glEnd();
    } break;

    case WATER: { // light blue
        glColor3d(0.55, 0.75, 0.98);
        glBegin(GL_POLYGON);
        glVertex2d(x0, y0); glVertex2d(x0, y1);
        glVertex2d(x1, y1); glVertex2d(x1, y0);
        glEnd();
        glColor3d(0, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2d(x0, y0); glVertex2d(x0, y1);
        glVertex2d(x1, y1); glVertex2d(x1, y0);
        glEnd();
    } break;

    case TREE: { // green triangle
        glColor3d(0.0, 0.45, 0.0);
        glBegin(GL_TRIANGLES);
        glVertex2d(c + 0.5, r + 1 - inset);
        glVertex2d(c + inset, r + inset);
        glVertex2d(c + 1 - inset, r + inset);
        glEnd();
        glColor3d(0, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2d(c + 0.5, r + 1 - inset);
        glVertex2d(c + inset, r + inset);
        glVertex2d(c + 1 - inset, r + inset);
        glEnd();
    } break;

    case SUPPLY_AMMO:
    case SUPPLY_MED: { // yellow squares
        glColor3d(0.98, 0.90, 0.15);
        glBegin(GL_POLYGON);
        glVertex2d(x0, y0); glVertex2d(x0, y1);
        glVertex2d(x1, y1); glVertex2d(x1, y0);
        glEnd();
        glColor3d(0, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2d(x0, y0); glVertex2d(x0, y1);
        glVertex2d(x1, y1); glVertex2d(x1, y0);
        glEnd();
    } break;
    }
}

void Map::draw() const {
    for (int i = 0; i < MSZ; ++i)
        for (int j = 0; j < MSZ; ++j)
            drawCell(i, j);
}

// ============================================================
// Line of Sight - Bresenham grid tracing
// ============================================================
bool Map::hasLineOfSight(const Vec2i& a, const Vec2i& b) const {
    int r0 = a.r, c0 = a.c;
    int r1 = b.r, c1 = b.c;

    int dr = std::abs(r1 - r0);
    int dc = std::abs(c1 - c0);
    int sr = (r0 < r1) ? 1 : -1;
    int sc = (c0 < c1) ? 1 : -1;
    int err = dr - dc;

    int r = r0, c = c0;
    while (true) {
        if (!(r == r0 && c == c0)) {
            CellType ct = at(r, c);
            if (BlocksVision(ct))
                return false;
        }

        if (r == r1 && c == c1)
            break;

        int e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r += sr; }
        if (e2 < dr) { err += dr; c += sc; }
    }
    return true;
}

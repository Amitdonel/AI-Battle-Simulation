#pragma once
#include "Definitions.h"
#include "Map.h"
#include "Types.h"
#include <cmath>
#include "glut.h"

// ------------------------------------------------------------
// Bullet class - simple projectile logic with rendering
// ------------------------------------------------------------
const double BULLET_SPEED = 0.1;

class Bullet {
private:
    // --- Position and direction ---
    double x, y;
    double dirX, dirY;

    // --- State ---
    bool isMoving = false;
    int life = 40; // number of frames before bullet disappears

public:
    // --------------------------------------------------------
    // Constructor: initializes bullet direction and activation
    // --------------------------------------------------------
    Bullet(double startX, double startY, double targetX, double targetY) {
        x = startX;
        y = startY;

        double dx = targetX - startX;
        double dy = targetY - startY;
        double len = std::sqrt(dx * dx + dy * dy);

        if (len < 0.001) len = 0.001; // prevent division by zero

        dirX = dx / len;
        dirY = dy / len;
        isMoving = true;
    }

    // --------------------------------------------------------
    // Accessors
    // --------------------------------------------------------
    void setMoving(bool value) { isMoving = value; }
    bool isAlive() const { return isMoving && life > 0; }

    // --------------------------------------------------------
    // Update: moves the bullet forward and checks collisions
    // --------------------------------------------------------
    void move(const Map& world) {
        if (!isMoving) return;

        // Move bullet by direction and speed
        x += dirX * BULLET_SPEED;
        y += dirY * BULLET_SPEED;
        life--;

        // Stop if out of bounds
        if (x < 0 || x >= MSZ || y < 0 || y >= MSZ) {
            isMoving = false;
            return;
        }

        // Stop if hits a blocking tile
        CellType ct = world.at((int)y, (int)x);
        if (ct == ROCK || ct == WATER)
            isMoving = false;
    }

    // --------------------------------------------------------
    // Render: draws the bullet as a red diamond shape
    // --------------------------------------------------------
    void draw() const {
        if (!isMoving) return;

        glColor3d(1.0, 0.1, 0.1); // bright red
        glBegin(GL_POLYGON);
        glVertex2d(x - 0.15, y);
        glVertex2d(x, y + 0.15);
        glVertex2d(x + 0.15, y);
        glVertex2d(x, y - 0.15);
        glEnd();
    }
};

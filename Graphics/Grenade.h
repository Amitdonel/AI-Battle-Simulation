#pragma once
#include "Bullet.h"
#include "Definitions.h"
#include "glut.h"
#include <cmath>

// ============================================================
// Grenade class - handles explosion bullets and visual effect
// ============================================================
const int NUM_GRENADE_BULLETS = 20;

class Grenade {
private:
    // --- Position and state ---
    double x, y;
    bool isExploding = false;

    // --- Explosion fragments ---
    Bullet* bullets[NUM_GRENADE_BULLETS];

public:
    // --------------------------------------------------------
    // Constructor: creates bullets radiating in all directions
    // --------------------------------------------------------
    Grenade(double posX, double posY) {
        double angleStep = 2 * 3.14159265 / NUM_GRENADE_BULLETS;
        for (int i = 0; i < NUM_GRENADE_BULLETS; ++i) {
            double angle = i * angleStep;
            double tx = posX + std::cos(angle);
            double ty = posY + std::sin(angle);
            bullets[i] = new Bullet(posX, posY, tx, ty);
        }
    }

    // --------------------------------------------------------
    // Destructor: clean up bullet memory
    // --------------------------------------------------------
    ~Grenade() {
        for (int i = 0; i < NUM_GRENADE_BULLETS; ++i)
            delete bullets[i];
    }

    // --------------------------------------------------------
    // Control
    // --------------------------------------------------------
    void setExploding(bool value) {
        isExploding = value;
        for (int i = 0; i < NUM_GRENADE_BULLETS; ++i)
            bullets[i]->setMoving(value);
    }

    bool isActive() const { return isExploding; }

    // --------------------------------------------------------
    // Explosion update - moves and renders all bullets
    // --------------------------------------------------------
    void explode(const Map& world) {
        if (!isExploding) return;

        bool anyAlive = false;
        for (int i = 0; i < NUM_GRENADE_BULLETS; ++i) {
            bullets[i]->move(world);
            bullets[i]->draw();
            if (bullets[i]->isAlive())
                anyAlive = true;
        }

        // Stop explosion when all bullets are done
        if (!anyAlive)
            isExploding = false;
    }

    // --------------------------------------------------------
    // Draw all bullet fragments (for debugging or persistence)
    // --------------------------------------------------------
    void draw() const {
        for (int i = 0; i < NUM_GRENADE_BULLETS; ++i)
            bullets[i]->draw();
    }
};

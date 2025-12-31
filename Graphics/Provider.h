#pragma once
#include "Agent.h"
#include "Order.h"
#include "Types.h"
#include <vector>

// Forward declarations
class Map;
class Agent;
class Warrior;

// ============================================================
// Provider class - responsible for resupplying ammo and grenades
// to allied warriors.
// ============================================================
class Provider : public Agent {
public:
    Provider(TeamColor t, int r, int c);

    // --- Identity ---
    const char* roleLetter() const override { return "P"; }

    // --- Core behavior ---
    void update(Map& world) override;
    void receiveOrder(const Order& o) override;
    void onReachedStorage();

    // --- Key positions ---
    Vec2i homePos{ -1, -1 };        // spawn position (for returning)
    Vec2i ammoStorage{ -1, -1 };    // team's ammo warehouse
    Vec2i soldierTarget{ -1, -1 };  // warrior in need of ammo
    bool onReturn = false;          // true if heading to soldier
    bool reachedStorageOnce = false;// prevents repeated entry

private:
    Agent* targetPtr = nullptr;     // pointer to current soldier target

    // --- Internal helpers ---
    Agent* pickAmmoTarget(const Order& o);
    bool planPathTo(Map& world, const Vec2i& goal);
};

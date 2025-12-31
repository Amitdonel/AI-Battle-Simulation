#pragma once
#include "Agent.h"
#include <vector>

// Forward declarations
class Map;

// ============================================================
// Warrior class
// Combat unit responsible for attacking and defending.
// Can shoot bullets, throw grenades, and take cover.
// ============================================================
class Warrior : public Agent {
public:
    Warrior(TeamColor t, int r, int c);
    const char* roleLetter() const override { return "W"; }

    // --- Core behavior ---
    void update(Map& world) override;
    void receiveOrder(const Order& o) override;
    void tryAttackNearbyEnemies(const std::vector<Agent*>& enemies);

    // --- Grenade system ---
    int getGrenades() const { return grenades; }
    void useGrenade() { if (grenades > 0) grenades--; }
    void refillGrenades() { grenades = 3; }

    // --- Static render helpers ---
    static void updateBulletsAndDraw(Map& world);
    static void updateGrenadesAndDraw(Map& world);

private:
    // --- Combat states ---
    enum class CombatMode { NONE, ATTACKING, DEFENDING };
    enum class PeekState { HIDING, PEEKING, RETURNING };

    // --- Internal behavior ---
    void tickAttackLogic(Map& world);
    void tickDefendLogic(Map& world);
    Agent* findNearestVisibleEnemy(const std::vector<Agent*>& enemies) const;

    // --- Cover and vision helpers ---
    bool findBestCoverNear(int r0, int c0, const Map& world, int radius, int& outR, int& outC) const;

    // --- Combat parameters ---
    CombatMode mode = CombatMode::NONE;
    Vec2i rallyPoint = { -1, -1 };
    Vec2i defendPoint = { -1, -1 };
    Vec2i coverPos = { -1, -1 };

    PeekState peek = PeekState::HIDING;
    int peekTimer = 0;
    int retargetCounter = 0;

    // --- Constants ---
    static const int RETARGET_EVERY = 45;
    static const int WEAPON_RANGE_CELLS = 8;
    static const int FIRE_COOLDOWN_FRAMES = 75;
    static const int SEEK_COVER_RADIUS = 6;
    static const int PEEK_DURATION = 18;

    static const int DAMAGE_PER_SHOT = 12;
    static const int HIT_CHANCE_PERCENT = 40;
    static const int AMMO_COST_PER_SHOT = 2;

    int grenades = 3;      // initial grenade count
    int fireCooldown = 0;  // delay between shots
    int reloadTimer = 0; // counts frames when waiting near ammo

};

#pragma once
#include "Agent.h"
#include "Order.h"
#include "Types.h"
#include <vector>

// Forward declarations
class Map;
class Agent;

// ============================================================
// Medic class - responsible for reviving fallen teammates
// ============================================================
class Medic : public Agent {
public:
    Medic(TeamColor t, int r, int c);

    // --- Identity ---
    const char* roleLetter() const override { return "M"; }

    // --- Core behavior ---
    void update(Map& world) override;
    void receiveOrder(const Order& o) override;
    void onReachedStorage();

    // --- Key positions ---
    Vec2i medStorage{ -1, -1 };     // team medical warehouse
    Vec2i soldierTarget{ -1, -1 };  // target (fallen soldier)
    Vec2i homePos{ -1, -1 };        // spawn position (for returning)
    bool onReturn = false;          // true if returning to patient

private:
    Agent* patientPtr = nullptr;    // reference to soldier being revived

    // --- Internal helpers ---
    bool planPathTo(Map& world, const Vec2i& goal);
    Agent* pickWoundedTarget(const Order& o);
    std::vector<Agent*>& myTeamVec();
    std::vector<Agent*>& enemyTeamVec();
};

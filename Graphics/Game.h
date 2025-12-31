#pragma once
#include "Definitions.h"
#include "Map.h"
#include "SafetyMap.h"
#include <vector>
#include <string>

// Forward declaration
class Agent;

// ============================================================
// Game class - handles initialization, main loop, and rendering
// ============================================================
class Game {
private:
    // --- Map and simulation ---
    Map world;
    int frame = 0;

    // --- Team agents ---
    std::vector<Agent*> teamOrange;
    std::vector<Agent*> teamBlue;

    // --- Safety maps ---
    SafetyMap dangerOrange; // danger from blue team
    SafetyMap dangerBlue;   // danger from orange team

    // --- Storage positions ---
    Vec2i medStorageOrange;
    Vec2i ammoStorageOrange;
    Vec2i medStorageBlue;
    Vec2i ammoStorageBlue;

public:
    // --- Game state ---
    bool gameOver = false;
    std::string winningTeam = "";

public:
    // --- Core methods ---
    Game();
    void init();    // setup map, agents, and initial states
    void update();  // per-frame game logic
    void render() const; // render all entities

    // --- Accessor ---
    Map& getMap() { return world; }
};

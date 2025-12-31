#pragma once
#include <vector>

// ============================================================
// Globals.h
// External global references used across FSM states
// ============================================================

// --- Forward declarations ---
class Map;
class SafetyMap;
class Agent;

// --- Global map reference (for FSM pathfinding) ---
extern Map* gWorldForStates;

// --- Global safety maps (danger fields for each team) ---
extern SafetyMap* gDangerOrange;
extern SafetyMap* gDangerBlue;

// --- Team vectors (shared across the simulation) ---
extern std::vector<Agent*> gTeamOrange;
extern std::vector<Agent*> gTeamBlue;

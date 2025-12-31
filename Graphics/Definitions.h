#pragma once

// ============================================================
// Definitions.h
// Global constants and enumerations for the simulation
// ============================================================

// ----- Grid size -----
#ifndef MSZ
#define MSZ 40
#endif

// ----- Cell types -----
enum CellType : int {
    EMPTY = 0,        // free cell
    ROCK = 1,         // blocks movement, fire, and vision
    TREE = 2,         // allows movement, blocks fire and vision
    WATER = 3,        // blocks movement, allows fire and vision
    SUPPLY_AMMO = 4,  // team's ammo warehouse
    SUPPLY_MED = 5    // team's medical warehouse
};

// ----- Rendering -----
const int WIN_W = 800;
const int WIN_H = 800;

// ----- Math -----
const double PI = 3.141592653589793;

// ----- Teams -----
enum TeamColor : int {
    TEAM_ORANGE = 0,
    TEAM_BLUE = 1
};

// ----- Role letters (for on-screen display) -----
inline const char* RoleLetterCommander() { return "C"; }
inline const char* RoleLetterWarrior() { return "W"; }
inline const char* RoleLetterMedic() { return "M"; }
inline const char* RoleLetterProvider() { return "P"; }

// ----- Gameplay ranges (in tiles) -----
const int SIGHT_RANGE = 12;  // visual detection range
const int FIRE_RANGE = 8;   // weapon effective range
const int GRENADE_RANGE = 5;   // grenade throwing range

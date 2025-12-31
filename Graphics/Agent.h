#pragma once
#include "Definitions.h"
#include "Types.h"
#include "Order.h"
#include <vector>
#include <algorithm>
#include <cstdio>

class State;
class Map;

class Agent {
protected:
    // --- Core attributes ---
    TeamColor team;
    Vec2i pos;
    Vec2i target;
    bool moving = false;
    bool alive = true;

    // --- Stats ---
    double hp = 100.0;
    int bullets = 4;
    int grenades = 2;
    const int maxHP = 100;
    const int maxBullets = 4;

    // --- Pathfinding and state ---
    std::vector<Vec2i> path;
    int pathIndex = -1;
    int moveDelayCounter = 0;
    static const int MOVE_DELAY = 80;

    State* current = nullptr;
    State* interrupted = nullptr;

    // --- Visibility map ---
    std::vector<uint8_t> vis;

    // --- Internal helpers ---
    void takeDamage(int dmg) {
        hp = std::max(0.0, hp - dmg);
        if (hp <= 0.0) alive = false;
    }

    void heal() {
        hp = maxHP;
        alive = true;
    }

public:
    // --- Construction ---
    Agent(TeamColor t, int r, int c);
    virtual ~Agent();

    // --- Core behavior ---
    virtual void update(Map& world);
    virtual void render() const;

    // --- Targeting & Movement ---
    void setTarget(const Vec2i& v) { target = v; }
    void setTarget(int r, int c) { target = { r, c }; }
    void stepTowardTarget(const Map& world);
    bool advanceAlongPath(const Map& world);

    // --- State management ---
    void setState(State* s);
    void setInterrupted(State* s) { interrupted = s; }
    State* getState() { return current; }
    State* getInterrupted() { return interrupted; }

    // --- Path access ---
    void setPath(const std::vector<Vec2i>& p) {
        path = p;
        pathIndex = path.empty() ? -1 : 0;
        moving = !path.empty();
    }

    const std::vector<Vec2i>& getPath() const { return path; }
    int getPathIndex() const { return pathIndex; }

    // --- Position access ---
    int row() const { return pos.r; }
    int col() const { return pos.c; }
    const Vec2i& getPos() const { return pos; }
    const Vec2i& getTarget() const { return target; }

    // --- Team and identity ---
    virtual const char* roleLetter() const = 0;
    TeamColor getTeam() const { return team; }

    // --- Movement control ---
    bool isMoving() const { return moving; }
    void setMoving(bool v) { moving = v; }

    // --- Combat and health ---
    double getHP() const { return hp; }
    int getBullets() const { return bullets; }
    bool isAlive() const { return alive; }

    void reload() { bullets = maxBullets; }
    void reduceAmmo(int used) { bullets = std::max(0, bullets - used); }

    bool needsHeal() const { return hp < 40.0; }
    bool needsAmmo() const { return bullets < 5; }

    void healFull() {
        hp = 100.0;
        alive = true;
        moving = false;
        path.clear();
        pathIndex = -1;
        /*std::printf("💚 %s soldier revived at (%d,%d)\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"), pos.r, pos.c);*/
    }

    void reduceHP(double dmg) {
        hp -= dmg;
        if (hp <= 0.0) {
            hp = 0.0;
            alive = false;
            moving = false;
            path.clear();
            pathIndex = -1;

            if (current)
                current = nullptr;

            /*std::printf("💀 %s soldier died at (%d,%d)\n",
                (team == TEAM_ORANGE ? "Orange" : "Blue"), pos.r, pos.c);*/
        }
    }

    // --- Visibility system ---
    virtual int getSightRange() const { return SIGHT_RANGE; }
    void computeVisibility(const Map& world);

    bool canSee(int r, int c) const {
        if (r < 0 || r >= MSZ || c < 0 || c >= MSZ) return false;
        return !vis.empty() && vis[r * MSZ + c] != 0;
    }

    const std::vector<uint8_t>& getVisibility() const { return vis; }

    // --- Orders ---
    virtual void receiveOrder(const Order& o);
};

#include "Provider.h"
#include "MoveToTarget.h"
#include "Idle.h"
#include "Globals.h"
#include "Map.h"
#include "Pathfinder.h"
#include "SafetyMap.h"
#include "Warrior.h"
#include <cstdio>
#include <algorithm>

// ------------------------------------------------------------
// External globals
// ------------------------------------------------------------
extern std::vector<Agent*> gTeamOrange;
extern std::vector<Agent*> gTeamBlue;
extern SafetyMap* gDangerOrange;
extern SafetyMap* gDangerBlue;
extern Map* gWorldForStates;

// ------------------------------------------------------------
// Utility
// ------------------------------------------------------------
static inline bool inWorld(int r, int c) {
    return (r >= 0 && r < MSZ && c >= 0 && c < MSZ);
}

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
Provider::Provider(TeamColor t, int r, int c) : Agent(t, r, c) {
    homePos = { r, c };
}

// ------------------------------------------------------------
// Find a teammate that needs ammunition
// ------------------------------------------------------------
Agent* Provider::pickAmmoTarget(const Order& o) {
    auto& team = (getTeam() == TEAM_ORANGE) ? gTeamOrange : gTeamBlue;
    Agent* best = nullptr;
    int bestD = 1e9;

    Vec2i anchor = (o.targetRow >= 0 && o.targetCol >= 0)
        ? Vec2i{ o.targetRow, o.targetCol }
    : this->getPos();

    for (auto* a : team) {
        if (a == this) continue;
        if (auto* w = dynamic_cast<Warrior*>(a)) {
            if (w->isAlive() && w->getBullets() == 0) {
                int d = std::abs(a->row() - anchor.r) + std::abs(a->col() - anchor.c);
                if (d < bestD) { bestD = d; best = a; }
            }
        }
    }
    return best;
}

// ------------------------------------------------------------
// Plan a safe path using A* with optional danger map
// ------------------------------------------------------------
bool Provider::planPathTo(Map& world, const Vec2i& goal) {
    if (!inWorld(goal.r, goal.c)) return false;

    std::vector<Vec2i> p;
    const SafetyMap* sm = (getTeam() == TEAM_ORANGE) ? gDangerOrange : gDangerBlue;
    const int (*danger)[MSZ] = sm ? sm->getGridPtr() : nullptr;

    bool ok = Pathfinder::AStar(world, getPos(), goal, p, danger);
    if (!ok || p.empty()) {
        ok = Pathfinder::AStar(world, getPos(), goal, p, nullptr);
        if (!ok || p.empty()) return false;
    }

    setPath(p);
    setTarget(goal);
    return true;
}

// ------------------------------------------------------------
// Receive order from Commander (RESUPPLY type)
// ------------------------------------------------------------
void Provider::receiveOrder(const Order& o) {
    if (o.type != OrderType::RESUPPLY) {
        setState(new Idle());
        return;
    }

    if (o.targetRow < 0 || o.targetCol < 0) {
        /*printf("Provider (%s): invalid order target.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        return;
    }

    if (!inWorld(ammoStorage.r, ammoStorage.c)) {
     /*   printf("Provider (%s): ammoStorage invalid.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        return;
    }

    targetPtr = pickAmmoTarget(o);
    if (!targetPtr) {
        /*printf("Provider (%s): no soldier needs resupply.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        return;
    }

    soldierTarget = targetPtr->getPos();

    if (!gWorldForStates || !planPathTo(*gWorldForStates, ammoStorage)) {
        /*printf("Provider (%s): path to storage failed.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        return;
    }

    onReturn = false;
    reachedStorageOnce = false;
    setState(new MoveToTarget());
    moving = true;

    /*printf("Provider (%s): RESUPPLY → storage (%d,%d) → soldier (%d,%d)\n",
        (team == TEAM_ORANGE ? "Orange" : "Blue"),
        ammoStorage.r, ammoStorage.c, soldierTarget.r, soldierTarget.c);*/
}

// ------------------------------------------------------------
// Called when Provider reaches the ammo storage
// ------------------------------------------------------------
void Provider::onReachedStorage() {
    if (!targetPtr || !targetPtr->isAlive()) {
        /*printf("Provider (%s): invalid soldier after storage.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        moving = false;
        return;
    }

    // Skip if soldier is standing inside the storage cell
    soldierTarget = targetPtr->getPos();
    if (soldierTarget.r == getPos().r && soldierTarget.c == getPos().c) {
        /*printf("Provider (%s): soldier is at storage location — skipping move.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        onReturn = false;
        setState(new Idle());
        moving = false;
        return;
    }

    // Plan path from storage to soldier
    if (!gWorldForStates || !planPathTo(*gWorldForStates, soldierTarget)) {
        /*printf("Provider (%s): path storage→soldier failed.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        moving = false;
        return;
    }

    // Safety check for very short paths
    if (getPath().size() <= 1) {
        /*printf("Provider (%s): path too short, staying put.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        moving = false;
        return;
    }

    pathIndex = 0;
    onReturn = true;
    moving = true;

    setState(new MoveToTarget());
   /* printf("Provider (%s): leaving storage, heading to soldier at (%d,%d) pathLen=%zu\n",
        (team == TEAM_ORANGE ? "Orange" : "Blue"),
        soldierTarget.r, soldierTarget.c, getPath().size());*/
}

// ------------------------------------------------------------
// Frame update logic
// ------------------------------------------------------------
void Provider::update(Map& world) {
    if (!isAlive()) return;
    Agent::update(world);

    static int pathRecalcCooldown = 0;
    if (pathRecalcCooldown > 0) pathRecalcCooldown--;


    if (pathIndex >= 0) {
        bool walking = advanceAlongPath(world);
        if (walking) return;

        // --- Arrived at destination ---
        if (!onReturn) {
            if (!reachedStorageOnce) {
                reachedStorageOnce = true;
                onReachedStorage();
            }
            return;
        }
        else {
            // 🧭 keep tracking soldier's live position while returning
            if (targetPtr && targetPtr->isAlive()) {
                Vec2i livePos = targetPtr->getPos();

                // if soldier moved far from last known target, replan path dynamically
                int drift = std::abs(livePos.r - soldierTarget.r) + std::abs(livePos.c - soldierTarget.c);
                if (drift >= 2) {
                    soldierTarget = livePos;
                    if (pathRecalcCooldown == 0 && gWorldForStates && planPathTo(*gWorldForStates, soldierTarget)) {
                        pathRecalcCooldown = 60; // ~1 second
                        setTarget(soldierTarget);
                        setState(new MoveToTarget());
                        moving = true;
                        return;
                    }
                }

            }

            // --- Reached target soldier ---
            if (targetPtr && targetPtr->isAlive()) {
                if (auto* w = dynamic_cast<Warrior*>(targetPtr)) {
                    w->reload();
                    w->refillGrenades();
                    w->setState(new Idle());
                    /*printf("🔋 Provider (%s): resupplied Warrior at (%d,%d)\n",
                        (team == TEAM_ORANGE ? "Orange" : "Blue"),
                        soldierTarget.r, soldierTarget.c);*/


                    
                    auto& enemyTeam = (team == TEAM_ORANGE) ? gTeamBlue : gTeamOrange;
                    bool anyEnemyAlive = false;
                    for (auto* e : enemyTeam)
                        if (e->isAlive()) { anyEnemyAlive = true; break; }

                    if (anyEnemyAlive) {
                        
                        int r = (team == TEAM_ORANGE) ? MSZ - 8 : 8;
                        int c = (team == TEAM_ORANGE) ? MSZ - 8 : 8;
                        w->receiveOrder(Order(OrderType::ATTACK, r, c));
                        /*printf("⚔️ Warrior (%s): resupplied and resuming mission!\n",
                            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
                    }
                    else {
                       
                        int r = (team == TEAM_ORANGE) ? MSZ - 5 : 5;
                        int c = (team == TEAM_ORANGE) ? MSZ - 5 : 5;
                        w->receiveOrder(Order(OrderType::MOVE, r, c));
                        /*printf("🏁 Warrior (%s): no enemies left, advancing forward.\n",
                            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
                    }
                }
            }


            // Plan path back home
            if (gWorldForStates && planPathTo(*gWorldForStates, homePos)) {
                setTarget(homePos);
                setState(new MoveToTarget());
                moving = true;
                onReturn = false;
                /*printf("🏠 Provider (%s): returning home.\n",
                    (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
            }
            else {
                /*printf("Provider (%s): failed to plan path home.\n",
                    (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
                setState(new Idle());
                moving = false;
                onReturn = false;
            }

            targetPtr = nullptr;
            soldierTarget = { -1, -1 };
        }
    }
    // 🧠 NEW: automatic resupply check when idle
    //if (!isMoving()) {
    //    TeamColor team = getTeam(); // ✅ added line
    //    auto& teamVec = (team == TEAM_ORANGE) ? gTeamOrange : gTeamBlue;
    //    for (auto* a : teamVec) {
    //        if (auto* w = dynamic_cast<Warrior*>(a)) {
    //            if (w->isAlive() && w->getBullets() == 0) {
    //                soldierTarget = w->getPos();
    //                if (pathRecalcCooldown == 0 && gWorldForStates && planPathTo(*gWorldForStates, ammoStorage)) {
    //                    pathRecalcCooldown = 200;
    //                    onReturn = false;
    //                    reachedStorageOnce = false;
    //                    setState(new MoveToTarget());
    //                    moving = true;
    //                }
    //                break;
    //            }

    //        }
    //    }
    //}

}

#include "Medic.h"
#include "MoveToTarget.h"
#include "Idle.h"
#include "Globals.h"
#include "Map.h"
#include "Pathfinder.h"
#include "SafetyMap.h"
#include "Commander.h"
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
Medic::Medic(TeamColor t, int r, int c) : Agent(t, r, c) {
    homePos = { r, c };
}

// ------------------------------------------------------------
// Retrieve current team vector
// ------------------------------------------------------------
std::vector<Agent*>& Medic::myTeamVec() {
    return (getTeam() == TEAM_ORANGE) ? gTeamOrange : gTeamBlue;
}

// ------------------------------------------------------------
// Select closest dead teammate (for commander order)
// ------------------------------------------------------------
Agent* Medic::pickWoundedTarget(const Order& o) {
    auto& team = myTeamVec();
    Agent* best = nullptr;
    int bestD = 1e9;

    Vec2i anchor = (o.targetRow >= 0 && o.targetCol >= 0)
        ? Vec2i{ o.targetRow, o.targetCol }
    : this->getPos();

    for (auto* a : team) {
        if (a == this) continue;
        if (!a->isAlive() && a->getHP() <= 0.0) {
            int d = std::abs(a->row() - anchor.r) + std::abs(a->col() - anchor.c);
            if (d < bestD) { bestD = d; best = a; }
        }
    }
    return best;
}

// ------------------------------------------------------------
// Path planning using A* with optional danger map
// ------------------------------------------------------------
bool Medic::planPathTo(Map& world, const Vec2i& goal) {
    if (!inWorld(goal.r, goal.c)) return false;

    std::vector<Vec2i> path;
    const SafetyMap* sm = (getTeam() == TEAM_ORANGE) ? gDangerOrange : gDangerBlue;
    const int (*danger)[MSZ] = sm ? sm->getGridPtr() : nullptr;

    bool ok = Pathfinder::AStar(world, getPos(), goal, path, danger);
    if (!ok || path.empty()) {
        ok = Pathfinder::AStar(world, getPos(), goal, path, nullptr);
        if (!ok || path.empty()) return false;
    }

    setPath(path);
    setTarget(goal);
    return true;
}

// ------------------------------------------------------------
// Receive commander order (HEAL logic)
// ------------------------------------------------------------
void Medic::receiveOrder(const Order& o) {
    if (o.type != OrderType::HEAL) {
        Agent::receiveOrder(o);
        return;
    }

    if (o.targetRow < 0 || o.targetCol < 0) {
        /*std::printf("Medic (%s): invalid order target.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        return;
    }

    if (!inWorld(medStorage.r, medStorage.c)) {
        /*std::printf("Medic (%s): medStorage invalid.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        return;
    }

    patientPtr = pickWoundedTarget(o);
    if (!patientPtr || patientPtr->isAlive()) {
        /*std::printf("Medic (%s): no valid dead soldier found.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        return;
    }

    soldierTarget = patientPtr->getPos();

    if (!gWorldForStates || !planPathTo(*gWorldForStates, medStorage)) {
       /* std::printf("Medic (%s): path to storage failed.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        return;
    }

    onReturn = false;
    setState(new MoveToTarget());
    moving = true;

    /*std::printf("Medic (%s): moving to storage (%d,%d) → then revive (%d,%d)\n",
        (team == TEAM_ORANGE ? "Orange" : "Blue"),
        medStorage.r, medStorage.c, soldierTarget.r, soldierTarget.c);*/
}

// ------------------------------------------------------------
// Once medic reaches the medical storage
// ------------------------------------------------------------
void Medic::onReachedStorage() {
    if (!patientPtr || patientPtr->isAlive()) {
        // look for wounded teammate
        auto& teamVec = myTeamVec();
        Agent* wounded = nullptr;
        int bestDist = 1e9;

        for (auto* a : teamVec) {
            if (a == this) continue;
            if (a->isAlive() && a->getHP() < 50.0) {
                int d = std::abs(a->row() - row()) + std::abs(a->col() - col());
                if (d < bestDist) { bestDist = d; wounded = a; }
            }
        }

        if (wounded && wounded->isAlive() && wounded->getHP() < 100.0) {
            patientPtr = wounded;
            soldierTarget = wounded->getPos();

            if (gWorldForStates && planPathTo(*gWorldForStates, soldierTarget)) {
                onReturn = true;
                setState(new MoveToTarget());
                moving = true;
                /*printf("🏃 Medic (%s): switching to heal wounded soldier at (%d,%d)\n",
                    (team == TEAM_ORANGE ? "Orange" : "Blue"),
                    soldierTarget.r, soldierTarget.c);*/
                return;
            }
        }

        // no wounded found → go home
        /*printf("Medic (%s): no wounded to heal → returning home.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        planPathTo(*gWorldForStates, homePos);
        setState(new MoveToTarget());
        moving = true;
        onReturn = false;
        return;
    }

    soldierTarget = patientPtr->getPos();
    if (!gWorldForStates || !planPathTo(*gWorldForStates, soldierTarget)) {
        /*std::printf("Medic (%s): path storage→patient failed.\n",
            (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
        setState(new Idle());
        return;
    }

    onReturn = true;
    setState(new MoveToTarget());
    moving = true;

    /*std::printf("Medic (%s): heading to revive at (%d,%d)\n",
        (team == TEAM_ORANGE ? "Orange" : "Blue"),
        soldierTarget.r, soldierTarget.c);*/
}

// ------------------------------------------------------------
// Update cycle (executed each frame)
// ------------------------------------------------------------
void Medic::update(Map& world) {
    if (!isAlive()) return;

    // clear invalid pointer
    if (patientPtr && (!patientPtr->isAlive() || patientPtr->getHP() >= 100.0))
        patientPtr = nullptr;

    Agent::update(world);

    static int pathRecalcCooldown = 0;
    if (pathRecalcCooldown > 0) pathRecalcCooldown--;


    if (pathIndex >= 0) {
        bool walking = advanceAlongPath(world);
        if (walking) return;

        // --- Path completed ---
        if (!onReturn) {
            onReachedStorage();
            return;
        }
        else {
            // follow moving wounded soldier
            if (patientPtr && patientPtr->isAlive() && patientPtr->getHP() < 100.0) {
                Vec2i livePos = patientPtr->getPos();
                int drift = std::abs(livePos.r - soldierTarget.r) + std::abs(livePos.c - soldierTarget.c);
                if (drift >= 2) {
                    soldierTarget = livePos;
                    if (pathRecalcCooldown == 0 && gWorldForStates && planPathTo(*gWorldForStates, soldierTarget)) {
                        pathRecalcCooldown = 60;
                        setTarget(soldierTarget);
                        setState(new MoveToTarget());
                        moving = true;
                        return;
                    }
                }
            }

            // --- Reached wounded or fallen soldier ---
            if (patientPtr && patientPtr->isAlive() && patientPtr->getHP() < 100.0) {
                patientPtr->healFull();
                /*printf("💚 Medic (%s): healed wounded soldier at (%d,%d)\n",
                    (team == TEAM_ORANGE ? "Orange" : "Blue"),
                    soldierTarget.r, soldierTarget.c);*/
            }

            // re-engage after heal
            auto& teamVec = myTeamVec();
            for (auto* a : teamVec) {
                if (auto* cmd = dynamic_cast<Commander*>(a)) {
                    const Vec2i& p = patientPtr->getPos();
                    cmd->addOrder(Order(OrderType::ATTACK, p.r, p.c));
                    /*std::printf("⚔️ Commander %s re-engages healed warrior (%d,%d)\n",
                        (team == TEAM_ORANGE ? "Orange" : "Blue"), p.r, p.c);*/
                    break;
                }
            }

            // go home
            if (gWorldForStates && planPathTo(*gWorldForStates, homePos)) {
                setTarget(homePos);
                setState(new MoveToTarget());
                moving = true;
            }
            else {
                /*std::printf("Medic (%s): failed to plan path home.\n",
                    (team == TEAM_ORANGE ? "Orange" : "Blue"));*/
                setState(new Idle());
            }

            onReturn = false;
            patientPtr = nullptr;
            soldierTarget = { -1, -1 };
        }
    }

    // --- automatic heal logic when idle ---
    /*if (!isMoving()) {
        TeamColor team = getTeam();
        auto& teamVec = (team == TEAM_ORANGE) ? gTeamOrange : gTeamBlue;

        for (auto* a : teamVec) {
            if (auto* w = dynamic_cast<Warrior*>(a)) {
                if (w->isAlive() && w->getHP() < 50.0) {
                    patientPtr = w;
                    soldierTarget = w->getPos();

                    if (pathRecalcCooldown == 0 && gWorldForStates && planPathTo(*gWorldForStates, medStorage)) {
                        pathRecalcCooldown = 200;
                        onReturn = false;
                        setState(new MoveToTarget());
                        moving = true;
                        onReachedStorage();
                    }
                    break;
                }

            }
        }
    }*/

    // 🧠 Only act if commander didn't give an order
    if (!isMoving() && !patientPtr) {
        TeamColor team = getTeam();
        auto& teamVec = (team == TEAM_ORANGE) ? gTeamOrange : gTeamBlue;

        // 🏥 Base (medical storage) per team
        Vec2i baseStorage = (team == TEAM_ORANGE)
            ? Vec2i{ 6, 9 }
        : Vec2i{ MSZ - 7, MSZ - 10 };

        for (auto* a : teamVec) {
            if (auto* w = dynamic_cast<Warrior*>(a)) {
                if (w->isAlive() && w->getHP() < 50.0) {

                    // ⚖️ Calculate distance between wounded and base
                    int distToBase = std::abs(w->row() - baseStorage.r) +
                        std::abs(w->col() - baseStorage.c);

                    // 📏 Only start moving if soldier is near base (radius ≤ 5)
                    if (distToBase <= 3) {
                        patientPtr = w;
                        soldierTarget = w->getPos();

                        // 🏃 Step 1: go to medical storage first
                        if (gWorldForStates && planPathTo(*gWorldForStates, medStorage)) {
                            onReturn = false;
                            setState(new MoveToTarget());
                            moving = true;

                            /*std::printf("💊 Medic (%s): soldier near base (dist=%d) → going to med storage first\n",
                                team == TEAM_ORANGE ? "Orange" : "Blue", distToBase);*/
                        }

                        // 🔄 Step 2: once reaches storage, onReachedStorage() will handle moving to patient
                        // (it already calls planPathTo(soldierTarget))
                    }
                    else {
                        /*std::printf("⏳ Medic (%s): soldier too far (dist=%d) → waiting near base\n",
                            team == TEAM_ORANGE ? "Orange" : "Blue", distToBase);*/
                    }

                    break; // only handle one wounded soldier at a time
                }
            }
        }
    }


}

#include "Commander.h"
#include "Globals.h"
#include "Order.h"
#include "Agent.h"
#include "Warrior.h"
#include "Medic.h"
#include "Provider.h"
#include "Idle.h"
#include "Map.h"
#include "SafetyMap.h"
#include "Pathfinder.h"
#include "MoveToTarget.h"

#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "glut.h"

// ------------------------------------------------------------
// External references
// ------------------------------------------------------------
extern std::vector<Agent*> gTeamOrange;
extern std::vector<Agent*> gTeamBlue;
extern SafetyMap* gDangerOrange;
extern SafetyMap* gDangerBlue;

// ------------------------------------------------------------
// Utility
// ------------------------------------------------------------
template <typename T>
static inline T clampValue(T v, T lo, T hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
Commander::Commander(TeamColor t, int r, int c) : Agent(t, r, c) {
    srand((unsigned)time(nullptr));
}

// ------------------------------------------------------------
// Order Management API
// ------------------------------------------------------------
void Commander::addOrder(const Order& o) {
    orders.push_back(o); // normal priority
}

void Commander::addPriorityOrder(const Order& o) {
    orders.push_front(o); // high priority (used for HEAL)
}

bool Commander::hasPendingHeal() const {
    for (const auto& o : orders)
        if (o.type == OrderType::HEAL)
            return true;
    return false;
}

Order Commander::nextOrder() {
    if (orders.empty()) return Order();
    Order o = orders.front();
    orders.pop_front();
    return o;
}

// ------------------------------------------------------------
// Frame Update
// ------------------------------------------------------------
void Commander::update(Map& world) {
    Agent::update(world);

    std::vector<Agent*>& myTeam = (getTeam() == TEAM_ORANGE) ? gTeamOrange : gTeamBlue;
    std::vector<Agent*>& enemies = (getTeam() == TEAM_ORANGE) ? gTeamBlue : gTeamOrange;

    updateCombinedVisibility(myTeam);
    issueSupportOrders(myTeam);
    relocateIfInDanger(world, enemies);
}

// ------------------------------------------------------------
// Commander Logic (high-level AI decision-making)
// ------------------------------------------------------------
void Commander::updateCommanderLogic() {
    static int frames = 0;
    frames++;
    if (frames < 180) return;       // initial delay
    if (frames % 600 != 0) return;  // periodic update

    std::vector<Agent*>& enemies = (getTeam() == TEAM_ORANGE) ? gTeamBlue : gTeamOrange;
    std::vector<Agent*>& myTeam = (getTeam() == TEAM_ORANGE) ? gTeamOrange : gTeamBlue;

    // --- Check if all enemy warriors are eliminated ---
    bool enemyWarriorsAlive = false;
    for (auto* e : enemies)
        if (e->isAlive() && dynamic_cast<Warrior*>(e))
            enemyWarriorsAlive = true;

    if (!enemyWarriorsAlive) {
        bool anyEnemyAlive = false;
        for (auto* e : enemies) {
            if (!e->isAlive()) continue;
            addOrder(Order(OrderType::ATTACK, e->row(), e->col()));
            anyEnemyAlive = true;
        }

    
        if (!anyEnemyAlive) {
            // move forward ORANGE team to enemy base area
            int r = (getTeam() == TEAM_ORANGE) ? MSZ - 8 : 8;
            int c = (getTeam() == TEAM_ORANGE) ? MSZ - 8 : 8;
            addOrder(Order(OrderType::ATTACK, r, c));
            /*printf("🏁 Commander %s: all enemies dead → final assault towards enemy base!\n",
                getTeam() == TEAM_ORANGE ? "Orange" : "Blue");*/
        }


        /*printf("Commander %s: All enemy warriors down → ATTACK remaining units!\n",
            getTeam() == TEAM_ORANGE ? "Orange" : "Blue");*/
        return;
    }

    // --- If any enemies exist, ensure attack orders are present ---
    bool anyEnemyAlive = false;
    for (auto* e : enemies)
        if (e->isAlive()) {
            anyEnemyAlive = true;
            break;
        }

    if (anyEnemyAlive)
        addOrder(Order(OrderType::ATTACK, enemies[0]->row(), enemies[0]->col()));

    // --- Randomly issue strategic order ---
    int roll = rand() % 4;
    if (roll <= 1) {
        // 🎯 Randomized attack position per warrior
        int baseR = (getTeam() == TEAM_ORANGE) ? MSZ - 10 : 5;
        int baseC = (getTeam() == TEAM_ORANGE) ? MSZ - 10 : 5;

        // Add small random offset so each warrior takes a different path
        int offsetR = (rand() % 7) - 3;  // -3..+3
        int offsetC = (rand() % 7) - 3;

        int finalR = std::max(0, std::min(MSZ - 1, baseR + offsetR));
        int finalC = std::max(0, std::min(MSZ - 1, baseC + offsetC));

        addOrder(Order(OrderType::ATTACK, finalR, finalC));

        /*printf("⚔️ Commander %s issues ATTACK toward (%d,%d)\n",
            (getTeam() == TEAM_ORANGE ? "Orange" : "Blue"), finalR, finalC);*/
    }

    else if (roll == 2) {
        int baseR = (getTeam() == TEAM_ORANGE) ? 8 : MSZ - 8;
        int baseC = (getTeam() == TEAM_ORANGE) ? 8 : MSZ - 8;
        int offsetR = (rand() % 5) - 2;
        int offsetC = (rand() % 5) - 2;
        int finalR = std::max(0, std::min(MSZ - 1, baseR + offsetR));
        int finalC = std::max(0, std::min(MSZ - 1, baseC + offsetC));

        addOrder(Order(OrderType::DEFEND, finalR, finalC));

        /*printf("🛡️ Commander %s issues DEFEND at (%d,%d)\n",
            (getTeam() == TEAM_ORANGE ? "Orange" : "Blue"), finalR, finalC);*/
    }

}

// ------------------------------------------------------------
// Order Dispatching
// ------------------------------------------------------------
void Commander::dispatchOrders(std::vector<Agent*>& team) {
    if (orders.empty()) return;

    int healIndex = -1;
    for (int i = 0; i < (int)orders.size(); ++i)
        if (orders[i].type == OrderType::HEAL) {
            healIndex = i;
            break;
        }

    Order o;
    if (healIndex >= 0) {
        o = orders[healIndex];
        orders.erase(orders.begin() + healIndex);
    }
    else {
        o = nextOrder();
    }

    /*std::printf("Commander (%s) dispatching order: %d.\n",
        (getTeam() == TEAM_ORANGE ? "Orange" : "Blue"), (int)o.type);*/

    for (auto* a : team) {
        if (a == this) continue;
        char role = a->roleLetter()[0];

        if (o.type == OrderType::HEAL && role != 'M') continue;
        if (o.type == OrderType::RESUPPLY && role != 'P') continue;
        if ((o.type == OrderType::ATTACK || o.type == OrderType::DEFEND) && (role == 'M' || role == 'P'))
            continue;

        a->receiveOrder(o);
    }
}

// ------------------------------------------------------------
// Support Logic (Heal / Resupply)
// ------------------------------------------------------------
void Commander::issueSupportOrders(std::vector<Agent*>& team) {
    Medic* medic = nullptr;
    Provider* provider = nullptr;

    for (auto* a : team) {
        if (!medic && dynamic_cast<Medic*>(a)) medic = (Medic*)a;
        if (!provider && dynamic_cast<Provider*>(a)) provider = (Provider*)a;
    }

    // --- Find wounded soldier ---
    //Agent* wounded = nullptr;
    //double minHP = 100;
    //for (auto* a : team) {
    //    if (dynamic_cast<Medic*>(a) || dynamic_cast<Provider*>(a) || a == this) continue;
    //    if (a->getHP() > 0 && a->getHP() < minHP) {
    //        wounded = a;
    //        minHP = a->getHP();
    //    }
    //}

    //if (wounded && medic && minHP < 50.0) {

    //    // 🧠 Base (medical) storage per team
    //    Vec2i baseStorage = (getTeam() == TEAM_ORANGE)
    //        ? Vec2i{ 6, 9 }
    //    : Vec2i{ MSZ - 7, MSZ - 10 };

    //    // 🧮 Distance between wounded soldier and base
    //    int distToBase = std::abs(wounded->row() - baseStorage.r) +
    //        std::abs(wounded->col() - baseStorage.c);

    //    // 📏 Only send medic when soldier is near base (within 5 tiles)
    //    if (distToBase <= 8) {
    //        const Vec2i& p = wounded->getPos();
    //        addPriorityOrder(Order(OrderType::HEAL, p.r, p.c));

    //        /*std::printf("Commander %s orders Medic to HEAL (%d,%d) [dist=%d]\n",
    //            getTeam() == TEAM_ORANGE ? "Orange" : "Blue", p.r, p.c, distToBase);*/
    //    }
    //    else {
    //        /*std::printf("Commander %s: waiting — wounded soldier still far from base (dist=%d)\n",
    //            getTeam() == TEAM_ORANGE ? "Orange" : "Blue", distToBase);*/
    //    }
    //}


   // --- Find soldier low on ammo ---
    Agent* lowAmmo = nullptr;
    for (auto* a : team) {
        if (auto* w = dynamic_cast<Warrior*>(a)) {
            // if low on ammo (2 or less bullets)
            if (w->isAlive() && w->getBullets() <= 2) {
                lowAmmo = a;
                break;
            }
        }
    }

    if (lowAmmo && provider) {
        if (!provider->isMoving()) {

            // 🧠 Base position by team
            Vec2i baseStorage = (getTeam() == TEAM_ORANGE)
                ? Vec2i{ 6, 9 }
            : Vec2i{ MSZ - 7, MSZ - 10 };

            // 🧮 Distance between warrior and base
            int distToBase = std::abs(lowAmmo->row() - baseStorage.r) +
                std::abs(lowAmmo->col() - baseStorage.c);

            // 📏 Only send provider when warrior is near the base (within 5 tiles)
            if (distToBase <= 8) {
                const Vec2i& p = lowAmmo->getPos();
                addOrder(Order(OrderType::RESUPPLY, p.r, p.c));

                /*std::printf("Commander %s orders Provider to RESUPPLY near base (%d,%d)\n",
                    getTeam() == TEAM_ORANGE ? "Orange" : "Blue", p.r, p.c);*/
            }
            else {
                /*std::printf("Commander %s: waiting — Warrior still far from base (dist=%d)\n",
                    getTeam() == TEAM_ORANGE ? "Orange" : "Blue", distToBase);*/
            }
        }
    }

}

// ------------------------------------------------------------
// Visibility Merging
// ------------------------------------------------------------
void Commander::updateCombinedVisibility(const std::vector<Agent*>& team) {
    if (combinedVis.size() != size_t(MSZ * MSZ))
        combinedVis.assign(MSZ * MSZ, 0);

    std::fill(combinedVis.begin(), combinedVis.end(), 0);

    for (auto* a : team) {
        const auto& v = a->getVisibility();
        if (v.size() != size_t(MSZ * MSZ)) continue;
        for (size_t i = 0; i < v.size(); ++i)
            combinedVis[i] |= v[i];
    }
}

// ------------------------------------------------------------
// Relocation (danger avoidance using SafetyMap)
// ------------------------------------------------------------
void Commander::relocateIfInDanger(Map& world, const std::vector<Agent*>& enemies) {
    SafetyMap* myDanger = (getTeam() == TEAM_ORANGE) ? gDangerOrange : gDangerBlue;
    if (!myDanger) return;

    int dangerValue = myDanger->get(row(), col());
    const int DANGER_THRESHOLD = 8;
    if (dangerValue < DANGER_THRESHOLD) return;

    int bestR = row(), bestC = col(), bestVal = dangerValue;
    bool foundCover = false;

    // Scan for nearest safer cover
    for (int radius = 1; radius <= 5; ++radius) {
        for (int dr = -radius; dr <= radius; ++dr) {
            for (int dc = -radius; dc <= radius; ++dc) {
                int rr = row() + dr, cc = col() + dc;
                if (!world.inBounds(rr, cc)) continue;

                CellType ct = world.at(rr, cc);
                if (ct == ROCK || ct == WATER) continue;

                int val = myDanger->get(rr, cc);
                if (val >= bestVal) continue;

                // Check if cover exists nearby
                static const int adjR[4] = { -1, 1, 0, 0 };
                static const int adjC[4] = { 0, 0, -1, 1 };
                bool hasCover = false;
                for (int k = 0; k < 4; ++k) {
                    int nr = rr + adjR[k], nc = cc + adjC[k];
                    if (!world.inBounds(nr, nc)) continue;
                    CellType nt = world.at(nr, nc);
                    if (nt == ROCK || nt == TREE) {
                        hasCover = true;
                        break;
                    }
                }

                if (hasCover) {
                    bestR = rr; bestC = cc;
                    bestVal = val;
                    foundCover = true;
                }
            }
        }
        if (foundCover) break;
    }

    // Move to cover if found a safer spot
    if (bestVal < dangerValue && gWorldForStates) {
        std::vector<Vec2i> safePath;
        bool ok = Pathfinder::AStar(*gWorldForStates, getPos(), { bestR, bestC }, safePath);

        if (ok && !safePath.empty()) {
            setPath(safePath);
            setTarget({ bestR, bestC });
            setState(new MoveToTarget());
            moving = true;
            /*std::printf("🪨 Commander %s moving to cover (%d,%d) [danger %d→%d]\n",
                getTeam() == TEAM_ORANGE ? "Orange" : "Blue", bestR, bestC, dangerValue, bestVal);*/
        }
        else {
            /*std::printf("⚠️ Commander %s couldn't find a safe path, staying put.\n",
                getTeam() == TEAM_ORANGE ? "Orange" : "Blue");*/
        }
    }
}

// ------------------------------------------------------------
// Rendering
// ------------------------------------------------------------
void Commander::render() const {
    Agent::render();
}

const char* Commander::roleLetter() const {
    return "C";
}

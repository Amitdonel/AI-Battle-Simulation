#include "Warrior.h"
#include "Globals.h"
#include "Order.h"
#include "Map.h"
#include "MoveToTarget.h"
#include "Idle.h"
#include "Bullet.h"
#include "Grenade.h"
#include "Pathfinder.h"
#include <list>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <random>

extern std::vector<Agent*> gTeamOrange;
extern std::vector<Agent*> gTeamBlue;
extern Map* gWorldForStates;

static std::list<Grenade> gActiveGrenades;
static std::list<Bullet> gActiveBullets;

static inline bool isPassable(CellType ct) {
    return (ct != ROCK && ct != WATER);
}

Warrior::Warrior(TeamColor t, int r, int c) : Agent(t, r, c) {}

static std::mt19937& rng() {
    static std::mt19937 gen{ std::random_device{}() };
    return gen;
}

// ============================================================
// Update
// ============================================================
void Warrior::update(Map& world)
{
    Agent::update(world);
    if (!isAlive()) return;

    if (fireCooldown > 0) fireCooldown--;
    if (peekTimer > 0) peekTimer--;

    // 🧨 NEW: if out of ammo or low HP, seek safe area near base
    if (bullets == 0 || hp < 50.0) {
        // stop current action
        setMoving(false);

        // determine base storage area by team
        Vec2i baseStorage = (getTeam() == TEAM_ORANGE)
            ? Vec2i{ 6, 9 }                // orange base
        : Vec2i{ MSZ - 7, MSZ - 10 };  // blue base

        // pick random nearby offset (within radius 4)
        int radius = 2;
        int bestR = baseStorage.r + (rand() % (radius * 2 + 1) - radius);
        int bestC = baseStorage.c + (rand() % (radius * 2 + 1) - radius);

        // clamp inside world
        bestR = std::max(0, std::min(MSZ - 1, bestR));
        bestC = std::max(0, std::min(MSZ - 1, bestC));

        // make sure target is not blocked
        CellType ct = world.at(bestR, bestC);
        if (ct == ROCK || ct == WATER) {
            // fallback: exact storage location
            bestR = baseStorage.r;
            bestC = baseStorage.c;
        }

        // pathfind there
        std::vector<Vec2i> path;
        Pathfinder::AStar(world, getPos(), { bestR, bestC }, path);
        if (!path.empty()) {
            setPath(path);
            setTarget({ bestR, bestC });
            setState(new MoveToTarget());
            setMoving(true);
           /* if (bullets == 0)
                std::printf("🔫 %s Warrior out of ammo → heading to ammo area (%d,%d)\n",
                    getTeam() == TEAM_ORANGE ? "Orange" : "Blue", bestR, bestC);
            else
                std::printf("💔 %s Warrior low HP (%.0f) → retreating to safe area (%d,%d)\n",
                    getTeam() == TEAM_ORANGE ? "Orange" : "Blue", hp, bestR, bestC);*/
        }
        else {
            setState(new Idle());
           /* std::printf("⚠️ %s Warrior tried to retreat but no path found, staying put.\n",
                getTeam() == TEAM_ORANGE ? "Orange" : "Blue");*/
        }

        return;
    }



    if (mode == CombatMode::NONE) return;

    if (mode == CombatMode::ATTACKING)
        tickAttackLogic(world);
    else if (mode == CombatMode::DEFENDING)
        tickDefendLogic(world);
}

// ============================================================
// Receive orders
// ============================================================
void Warrior::receiveOrder(const Order& o)
{
    if (o.type == OrderType::ATTACK) {
        mode = CombatMode::ATTACKING;
        rallyPoint = { o.targetRow, o.targetCol };
        setTarget(rallyPoint);
        setState(new MoveToTarget());
        retargetCounter = 0;
        return;
    }

    if (o.type == OrderType::DEFEND) {
        mode = CombatMode::DEFENDING;
        defendPoint = { o.targetRow, o.targetCol };
        setTarget(defendPoint);
        setState(new MoveToTarget());
        retargetCounter = 0;
        return;
    }

    Agent::receiveOrder(o);
}

// ============================================================
// Find nearest visible enemy
// ============================================================
Agent* Warrior::findNearestVisibleEnemy(const std::vector<Agent*>& enemies) const
{
    Agent* best = nullptr;
    int bestD = 1e9;

    for (auto* e : enemies) {
        if (!e->isAlive()) continue;
        if (!canSee(e->row(), e->col())) continue;

        int d = std::abs(e->row() - row()) + std::abs(e->col() - col());
        if (d < bestD) { bestD = d; best = e; }
    }

    return best;
}

// ============================================================
// ATTACK MODE
// ============================================================
void Warrior::tickAttackLogic(Map& world)
{
    const std::vector<Agent*>& enemies =
        (getTeam() == TEAM_ORANGE) ? gTeamBlue : gTeamOrange;

    bool anyEnemyAlive = false;
    bool anyEnemyWarriorAlive = false;

    for (auto* e : enemies) {
        if (e->isAlive()) {
            anyEnemyAlive = true;
            if (dynamic_cast<Warrior*>(e))
                anyEnemyWarriorAlive = true;
        }
    }

    if (!anyEnemyAlive) return;

    std::vector<Agent*> validTargets;
    for (auto* e : enemies) {
        if (!e->isAlive()) continue;
        if (anyEnemyWarriorAlive && !dynamic_cast<Warrior*>(e)) continue;
        validTargets.push_back(e);
    }

    if (!anyEnemyWarriorAlive) {
        validTargets.clear();
        for (auto* e : enemies)
            if (e->isAlive()) validTargets.push_back(e);
    }

    Agent* bestEnemy = nullptr;
    int bestD = 1e9;

    for (auto* e : validTargets) {
        int d = std::abs(e->row() - row()) + std::abs(e->col() - col());
        if (d < bestD && gWorldForStates &&
            gWorldForStates->hasLineOfSight({ row(), col() }, { e->row(), e->col() })) {
            bestD = d;
            bestEnemy = e;
        }
    }

    if (bestEnemy) {
        setMoving(false);
        int dist = std::abs(bestEnemy->row() - row()) + std::abs(bestEnemy->col() - col());

        if (dist <= WEAPON_RANGE_CELLS &&
            gWorldForStates->hasLineOfSight({ row(), col() }, { bestEnemy->row(), bestEnemy->col() })) {

            int enemiesClose = 0;
            for (auto* e : validTargets) {
                if (!e->isAlive()) continue;
                int d = std::abs(e->row() - row()) + std::abs(e->col() - col());
                if (d <= 6) enemiesClose++;
            }

            if (enemiesClose >= 2 && grenades > 0 && fireCooldown == 0) {
                grenades--;
                gActiveGrenades.emplace_back(col() + 0.5, row() + 0.5);
                gActiveGrenades.back().setExploding(true);

                // 💥 Apply area damage (grenades do stronger AOE damage)
                const int GRENADE_DAMAGE = DAMAGE_PER_SHOT * 1.3;
                const int BLAST_RADIUS = 3; // Manhattan distance

                for (auto* e : enemies) {
                    if (!e->isAlive()) continue;
                    int dist = std::abs(e->row() - row()) + std::abs(e->col() - col());
                    if (dist <= BLAST_RADIUS) {
                        e->reduceHP(GRENADE_DAMAGE);
                        /*std::printf("💣 %s Warrior grenade hit enemy at (%d,%d) → enemy HP=%.0f\n",
                            getTeam() == TEAM_ORANGE ? "Orange" : "Blue",
                            e->row(), e->col(), e->getHP());*/
                    }
                }

                /*std::printf("💥 %s Warrior throws grenade! Remaining: %d\n",
                    getTeam() == TEAM_ORANGE ? "Orange" : "Blue", grenades);*/

                fireCooldown = FIRE_COOLDOWN_FRAMES * 2;
                return;
            }

            // Regular fire
            if (fireCooldown == 0 && bullets > 0) {
                bullets = std::max(0, bullets - AMMO_COST_PER_SHOT);
                bestEnemy->reduceHP(DAMAGE_PER_SHOT);
                gActiveBullets.emplace_back(col() + 0.5, row() + 0.5,
                    bestEnemy->col() + 0.5, bestEnemy->row() + 0.5);

                /*std::printf("%s Warrior fires → enemy HP=%.0f | bullets left=%d\n",
                    getTeam() == TEAM_ORANGE ? "Orange" : "Blue",
                    bestEnemy->getHP(), bullets);*/

                fireCooldown = FIRE_COOLDOWN_FRAMES;
            }
            return;
        }
    }

    Agent* closestAlive = nullptr;
    int minDist = 1e9;

    for (auto* e : validTargets) {
        if (!e->isAlive()) continue;
        int d = std::abs(e->row() - row()) + std::abs(e->col() - col());
        if (d < minDist) { minDist = d; closestAlive = e; }
    }

    if (closestAlive) {
        std::vector<Vec2i> path;
        Pathfinder::AStar(world, getPos(), closestAlive->getPos(), path);
        if (!path.empty()) setPath(path);
    }
}

// ============================================================
// DEFEND MODE
// ============================================================
void Warrior::tickDefendLogic(Map& world)
{
    const std::vector<Agent*>& enemies =
        (getTeam() == TEAM_ORANGE) ? gTeamBlue : gTeamOrange;

    Agent* bestEnemy = findNearestVisibleEnemy(enemies);
    if (!bestEnemy) return;

    int d = std::abs(bestEnemy->row() - row()) + std::abs(bestEnemy->col() - col());

    if (d <= WEAPON_RANGE_CELLS && gWorldForStates &&
        gWorldForStates->hasLineOfSight({ row(), col() }, { bestEnemy->row(), bestEnemy->col() })) {

        if (fireCooldown == 0 && bullets > 0) {
            bullets = std::max(0, bullets - AMMO_COST_PER_SHOT);
            bestEnemy->reduceHP(DAMAGE_PER_SHOT);
            gActiveBullets.emplace_back(col() + 0.5, row() + 0.5,
                bestEnemy->col() + 0.5, bestEnemy->row() + 0.5);

           /* std::printf("%s Warrior (DEFEND) fires → enemy HP=%.0f | bullets left=%d\n",
                getTeam() == TEAM_ORANGE ? "Orange" : "Blue",
                bestEnemy->getHP(), bullets);*/

            fireCooldown = FIRE_COOLDOWN_FRAMES;
        }
    }
}

// ============================================================
// STATIC DRAW HELPERS
// ============================================================
void Warrior::updateBulletsAndDraw(Map& world)
{
    for (auto it = gActiveBullets.begin(); it != gActiveBullets.end(); )
    {
        it->move(world);
        if (!it->isAlive()) it = gActiveBullets.erase(it);
        else { it->draw(); ++it; }
    }
}

void Warrior::updateGrenadesAndDraw(Map& world)
{
    for (auto it = gActiveGrenades.begin(); it != gActiveGrenades.end(); )
    {
        it->explode(world);
        if (!it->isActive()) it = gActiveGrenades.erase(it);
        else ++it;
    }
}

// ============================================================
// EXTRA FUNCTIONS
// ============================================================
void Warrior::tryAttackNearbyEnemies(const std::vector<Agent*>& enemies)
{
    if (fireCooldown > 0) return;

    Agent* best = nullptr;
    int bestD = 1e9;

    for (auto* e : enemies) {
        if (!e->isAlive()) continue;
        int d = std::abs(e->row() - row()) + std::abs(e->col() - col());
        if (d <= WEAPON_RANGE_CELLS && gWorldForStates &&
            gWorldForStates->hasLineOfSight({ row(),col() }, { e->row(),e->col() })) {
            if (d < bestD) { bestD = d; best = e; }
        }
    }

    if (best && best->isAlive() && bullets > 0) {
        bullets = std::max(0, bullets - AMMO_COST_PER_SHOT);
        best->reduceHP(DAMAGE_PER_SHOT);
        gActiveBullets.emplace_back(col() + 0.5, row() + 0.5,
            best->col() + 0.5, best->row() + 0.5);

    /*    std::printf("%s Warrior (tick) fires → enemy HP=%.0f | bullets left=%d\n",
            getTeam() == TEAM_ORANGE ? "Orange" : "Blue",
            best->getHP(), bullets);*/

        fireCooldown = FIRE_COOLDOWN_FRAMES;
    }
}

// ============================================================
// Find best cover cell nearby
// ============================================================
bool Warrior::findBestCoverNear(int r0, int c0, const Map& world, int radius,
    int& outR, int& outC) const
{
    int bestR = -1, bestC = -1, bestScore = -1000000000;

    for (int dr = -radius; dr <= radius; ++dr) {
        for (int dc = -radius; dc <= radius; ++dc) {
            int rr = r0 + dr, cc = c0 + dc;
            if (!world.inBounds(rr, cc)) continue;

            CellType ct = world.at(rr, cc);
            if (ct == ROCK || ct == WATER) continue;

            static const int adjR[4] = { -1, 1, 0, 0 };
            static const int adjC[4] = { 0, 0, -1, 1 };
            bool hasCover = false;

            for (int k = 0; k < 4; ++k) {
                int nr = rr + adjR[k], nc = cc + adjC[k];
                if (!world.inBounds(nr, nc)) continue;
                CellType nt = world.at(nr, nc);
                if (nt == ROCK || nt == TREE) { hasCover = true; break; }
            }

            if (hasCover) {
                int score = -(std::abs(rr - r0) + std::abs(cc - c0));
                if (score > bestScore) {
                    bestScore = score;
                    bestR = rr;
                    bestC = cc;
                }
            }
        }
    }

    if (bestR >= 0) {
        outR = bestR;
        outC = bestC;
        return true;
    }
    return false;
}

#include "Game.h"
#include "glut.h"
#include "Globals.h"
#include "Types.h"
#include "Agent.h"
#include "Commander.h"
#include "Warrior.h"
#include "Medic.h"
#include "Provider.h"
#include "SafetyMap.h"
#include "Grenade.h"
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <cstdio>
#include <typeinfo>
#include <type_traits>

// ------------------------------------------------------------
// Global definitions
// ------------------------------------------------------------
Map* gWorldForStates = nullptr;
SafetyMap* gDangerOrange = nullptr;
SafetyMap* gDangerBlue = nullptr;
std::vector<Agent*> gTeamOrange;
std::vector<Agent*> gTeamBlue;
bool gMedicBusy = false;
bool gProviderBusy = false;


// ------------------------------------------------------------
// Commander auto-heal helper
// ------------------------------------------------------------
static void commanderAutoHeal(std::vector<Agent*>& team) {
    Commander* cmd = nullptr;
    Medic* med = nullptr;

    for (auto* a : team) {
        if (!cmd)  cmd = dynamic_cast<Commander*>(a);
        if (!med)  med = dynamic_cast<Medic*>(a);
    }
    if (!cmd || !med) return;

    // Skip if commander already has heal queued or medic is busy
    if (cmd->hasPendingHeal()) return;
    if (med->isMoving()) return;

    // Find dead teammate
    Agent* bestDown = nullptr;
    for (auto* a : team) {
        if (a == med) continue;
        if (a->getHP() <= 0.0) { bestDown = a; break; }
    }

    if (!bestDown) return;

    const Vec2i& p = bestDown->getPos();
    cmd->addPriorityOrder(Order(OrderType::HEAL, p.r, p.c));
   /* std::printf("Commander auto-HEAL -> (%d,%d) [HP=%.0f].\n", p.r, p.c, bestDown->getHP());*/
}

// ------------------------------------------------------------
// Helper functions
// ------------------------------------------------------------
static bool occupied(const std::vector<Vec2i>& taken, int r, int c) {
    for (const auto& p : taken)
        if (p.r == r && p.c == c) return true;
    return false;
}

static Vec2i pickFreeInBox(Map& world, int r0, int r1, int c0, int c1, const std::vector<Vec2i>& taken) {
    for (int guard = 0; guard < 8000; ++guard) {
        int r = r0 + rand() % std::max(1, (r1 - r0 + 1));
        int c = c0 + rand() % std::max(1, (c1 - c0 + 1));
        CellType ct = world.at(r, c);
        if ((ct == EMPTY || ct == TREE) && !occupied(taken, r, c))
            return { r, c };
    }
    return { (r0 + r1) / 2, (c0 + c1) / 2 };
}

static void ensureClearRing(Map& world, const Vec2i& center, int radius = 2) {
    for (int dr = -radius; dr <= radius; ++dr)
        for (int dc = -radius; dc <= radius; ++dc) {
            int rr = center.r + dr;
            int cc = center.c + dc;
            if (!world.inBounds(rr, cc)) continue;

            CellType ct = world.at(rr, cc);
            if (ct == ROCK || ct == WATER)
                world.set(rr, cc, EMPTY);
        }
}

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
Game::Game() : frame(0) {}

// ------------------------------------------------------------
// Visibility merging for commander
// ------------------------------------------------------------
static void updateCommanderVisibilityForTeam(const std::vector<Agent*>& team) {
    Commander* cmd = nullptr;
    for (auto* a : team) {
        cmd = dynamic_cast<Commander*>(a);
        if (cmd) break;
    }

    if (!cmd) return;

    if (cmd->isAlive()) {
        std::vector<uint8_t> unionVis(MSZ * MSZ, 0);
        for (auto* a : team) {
            const auto& v = a->getVisibility();
            if (v.size() != size_t(MSZ * MSZ)) continue;
            for (size_t i = 0; i < v.size(); ++i)
                unionVis[i] |= v[i];
        }
        cmd->setCombinedVisibility(unionVis);
    }
    else {
        std::vector<uint8_t> emptyVis(MSZ * MSZ, 0);
        cmd->setCombinedVisibility(emptyVis);
    }
}

// ------------------------------------------------------------
// Initialization
// ------------------------------------------------------------
void Game::init() {
    srand((unsigned)time(nullptr));
    world.initStructured();

    // Define storages for both teams
    medStorageOrange = { 6, 6 };
    ammoStorageOrange = { 6, 9 };
    medStorageBlue = { MSZ - 7, MSZ - 7 };
    ammoStorageBlue = { MSZ - 7, MSZ - 10 };
    gWorldForStates = &world;

    // Connect global danger maps
    gDangerOrange = &dangerOrange;
    gDangerBlue = &dangerBlue;

    // Spawn regions
    int Or_r0 = 3, Or_r1 = MSZ / 3;
    int Or_c0 = 3, Or_c1 = MSZ / 3;
    int Bl_r0 = MSZ - MSZ / 3, Bl_r1 = MSZ - 3;
    int Bl_c0 = MSZ - MSZ / 3, Bl_c1 = MSZ - 3;

    std::vector<Vec2i> takenOrange, takenBlue;

    // --- Orange team ---
    {
        Vec2i pC = pickFreeInBox(world, Or_r0, Or_r1, Or_c0, Or_c1, takenOrange);
        Vec2i pW1 = pickFreeInBox(world, Or_r0, Or_r1, Or_c0, Or_c1, takenOrange);
        Vec2i pW2 = pickFreeInBox(world, Or_r0, Or_r1, Or_c0, Or_c1, takenOrange);
        Vec2i pM = pickFreeInBox(world, Or_r0, Or_r1, Or_c0, Or_c1, takenOrange);
        Vec2i pP = pickFreeInBox(world, Or_r0, Or_r1, Or_c0, Or_c1, takenOrange);

        auto* medicO = new Medic(TEAM_ORANGE, pM.r, pM.c);
        medicO->medStorage = medStorageOrange;
        teamOrange.push_back(medicO);

        auto* provO = new Provider(TEAM_ORANGE, pP.r, pP.c);
        provO->ammoStorage = ammoStorageOrange;
        teamOrange.push_back(provO);

        auto* cmdO = new Commander(TEAM_ORANGE, pC.r, pC.c);
        teamOrange.push_back(cmdO);
        ensureClearRing(world, pC, 2);

        teamOrange.push_back(new Warrior(TEAM_ORANGE, pW1.r, pW1.c));
        teamOrange.push_back(new Warrior(TEAM_ORANGE, pW2.r, pW2.c));
    }

    // --- Blue team ---
    {
        Vec2i pC = pickFreeInBox(world, Bl_r0, Bl_r1, Bl_c0, Bl_c1, takenBlue);
        Vec2i pW1 = pickFreeInBox(world, Bl_r0, Bl_r1, Bl_c0, Bl_c1, takenBlue);
        Vec2i pW2 = pickFreeInBox(world, Bl_r0, Bl_r1, Bl_c0, Bl_c1, takenBlue);
        Vec2i pM = pickFreeInBox(world, Bl_r0, Bl_r1, Bl_c0, Bl_c1, takenBlue);
        Vec2i pP = pickFreeInBox(world, Bl_r0, Bl_r1, Bl_c0, Bl_c1, takenBlue);

        auto* medicB = new Medic(TEAM_BLUE, pM.r, pM.c);
        medicB->medStorage = medStorageBlue;
        teamBlue.push_back(medicB);

        auto* provB = new Provider(TEAM_BLUE, pP.r, pP.c);
        provB->ammoStorage = ammoStorageBlue;
        teamBlue.push_back(provB);

        auto* cmdB = new Commander(TEAM_BLUE, pC.r, pC.c);
        teamBlue.push_back(cmdB);
        ensureClearRing(world, pC, 2);

        teamBlue.push_back(new Warrior(TEAM_BLUE, pW1.r, pW1.c));
        teamBlue.push_back(new Warrior(TEAM_BLUE, pW2.r, pW2.c));
    }

    // --- Initial orders ---
    for (auto* a : teamOrange)
        if (auto* cmd = dynamic_cast<Commander*>(a))
            cmd->addOrder(Order(OrderType::ATTACK, MSZ - 10, MSZ - 10));

    for (auto* a : teamBlue)
        if (auto* cmd = dynamic_cast<Commander*>(a))
            cmd->addOrder(Order(OrderType::ATTACK, 5, 5));

    gTeamOrange = teamOrange;
    gTeamBlue = teamBlue;
}

// ------------------------------------------------------------
// Update logic (main game loop)
// ------------------------------------------------------------
void Game::update() {
    
    ++frame;

    // 1. Compute danger maps
    dangerOrange.compute(teamBlue);
    dangerBlue.compute(teamOrange);

    // 2. Auto-heal if needed
    commanderAutoHeal(teamOrange);
    commanderAutoHeal(teamBlue);

    // 3. Commander logic
    Commander* orangeCmd = nullptr;
    Commander* blueCmd = nullptr;
    bool orangeAlive = false;
    bool blueAlive = false;

    for (auto* a : teamOrange)
        if (auto* cmd = dynamic_cast<Commander*>(a)) { orangeCmd = cmd; orangeAlive = cmd->isAlive(); }

    for (auto* a : teamBlue)
        if (auto* cmd = dynamic_cast<Commander*>(a)) { blueCmd = cmd; blueAlive = cmd->isAlive(); }

    if (orangeAlive) orangeCmd->updateCommanderLogic();
    if (blueAlive)   blueCmd->updateCommanderLogic();

    // Warriors act independently if commander is dead
    if (!orangeAlive)
        for (auto* a : teamOrange)
            if (auto* w = dynamic_cast<Warrior*>(a))
                w->tryAttackNearbyEnemies(gTeamBlue);

    if (!blueAlive)
        for (auto* a : teamBlue)
            if (auto* w = dynamic_cast<Warrior*>(a))
                w->tryAttackNearbyEnemies(gTeamOrange);

    // 4. Dispatch orders
    for (auto* a : teamOrange)
        if (auto* cmd = dynamic_cast<Commander*>(a))
            cmd->dispatchOrders(teamOrange);

    for (auto* a : teamBlue)
        if (auto* cmd = dynamic_cast<Commander*>(a))
            cmd->dispatchOrders(teamBlue);

    // 5. Update agents
    for (auto* a : teamOrange) a->update(world);
    for (auto* a : teamBlue)   a->update(world);

    // 6. Warrior combat
    for (auto* a : teamOrange)
        if (auto* w = dynamic_cast<Warrior*>(a))
            w->tryAttackNearbyEnemies(teamBlue);

    for (auto* a : teamBlue)
        if (auto* w = dynamic_cast<Warrior*>(a))
            w->tryAttackNearbyEnemies(teamOrange);

    // 7. Check victory condition
    bool allOrangeDead = std::all_of(teamOrange.begin(), teamOrange.end(), [](Agent* a) { return !a->isAlive(); });
    bool allBlueDead = std::all_of(teamBlue.begin(), teamBlue.end(), [](Agent* a) { return !a->isAlive(); });

    if (!gameOver && (allOrangeDead || allBlueDead)) {
        gameOver = true;
        if (allOrangeDead && !allBlueDead)
            winningTeam = "BLUE TEAM WINS!";
        else if (allBlueDead && !allOrangeDead)
            winningTeam = "ORANGE TEAM WINS!";
        else
            winningTeam = "🤝 DRAW!";

        printf("========================================\n");
        printf("🏆 %s\n", winningTeam.c_str());
        printf("========================================\n");
    }

    // 8. Update combined visibility
    updateCommanderVisibilityForTeam(teamOrange);
    updateCommanderVisibilityForTeam(teamBlue);
}

// ------------------------------------------------------------
// Rendering
// ------------------------------------------------------------
void Game::render() const {
    world.draw();

    for (auto* a : teamOrange) a->render();
    for (auto* a : teamBlue)   a->render();

    // Draw active bullets and grenades
    Warrior::updateBulletsAndDraw(const_cast<Map&>(world));
    Warrior::updateGrenadesAndDraw(const_cast<Map&>(world));

    // Display winner banner
    if (gameOver) {
        void* font = GLUT_BITMAP_TIMES_ROMAN_24;

        
        int textWidth = 0;
        for (char c : winningTeam)
            textWidth += glutBitmapWidth(font, c);

        double centerX = MSZ / 2.0;
        double centerY = MSZ / 2.0;

        
        double paddingX = 1.0;   
        double paddingY = 0.8;   
        double rectWidth = textWidth / 10.0 + paddingX * 2;
        double rectHeight = 2.5;

        double rectX0 = centerX - rectWidth / 2.0;
        double rectY0 = centerY - rectHeight / 2.0;
        double rectX1 = centerX + rectWidth / 2.0;
        double rectY1 = centerY + rectHeight / 2.0;

        
        glColor3d(1.0, 1.0, 1.0);
        glBegin(GL_POLYGON);
        glVertex2d(rectX0, rectY0);
        glVertex2d(rectX0, rectY1);
        glVertex2d(rectX1, rectY1);
        glVertex2d(rectX1, rectY0);
        glEnd();

        glColor3d(0.0, 0.0, 0.0);
        glBegin(GL_LINE_LOOP);
        glVertex2d(rectX0, rectY0);
        glVertex2d(rectX0, rectY1);
        glVertex2d(rectX1, rectY1);
        glVertex2d(rectX1, rectY0);
        glEnd();

       
        double textX = centerX - (textWidth / 2.0) / 10.0;
        double textY = centerY - 0.5; 
        glColor3d(0.0, 0.0, 0.0);
        glRasterPos2d(textX, textY);

        for (char c : winningTeam)
            glutBitmapCharacter(font, c);
    }

}

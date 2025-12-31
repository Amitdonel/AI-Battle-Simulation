#include "MoveToTarget.h"
#include "Idle.h"
#include "Agent.h"
#include "Map.h"
#include "Globals.h"
#include "Pathfinder.h"
#include "Types.h"
#include "SafetyMap.h"
#include "Medic.h"
#include "Provider.h"

// ============================================================
// OnEnter - calculate A* path to target
// ============================================================
void MoveToTarget::OnEnter(Agent* a) {
    std::vector<Vec2i> path;
    Vec2i start = a->getPos();
    Vec2i goal = a->getTarget();

    const int (*dangerGrid)[MSZ] = nullptr;

    // Use danger map only for combat units (not Medic or Provider)
    if (!dynamic_cast<Medic*>(a) && !dynamic_cast<Provider*>(a)) {
        if (a->getTeam() == TEAM_ORANGE && gDangerOrange)
            dangerGrid = gDangerOrange->getGridPtr();
        else if (a->getTeam() == TEAM_BLUE && gDangerBlue)
            dangerGrid = gDangerBlue->getGridPtr();
    }

    // Attempt to plan a safe path
    if (Pathfinder::AStar(*gWorldForStates, start, goal, path, dangerGrid))
        a->setPath(path);
    else {
        a->setPath({});
        a->setState(new Idle());
    }
}

// ============================================================
// Transition - move along path; handle completion
// ============================================================
void MoveToTarget::Transition(Agent* a) {
    // Continue moving until path ends
    if (!a->advanceAlongPath(*gWorldForStates)) {
        OnExit(a);

        // --- Special behaviors ---
        if (auto* m = dynamic_cast<Medic*>(a)) {
            if (!m->onReturn) {
                m->onReachedStorage();
                return;
            }
        }

        if (auto* p = dynamic_cast<Provider*>(a)) {
            if (!p->onReturn) {
                p->onReachedStorage();
                return;
            }
        }

        // Default: return to Idle
        a->setState(new Idle());
    }
}

// ============================================================
// OnExit - cleanup (currently unused)
// ============================================================
void MoveToTarget::OnExit(Agent* /*a*/) {
    // No cleanup required for this state
}

#pragma once
#include "State.h"

// ============================================================
// MoveToTarget State
// Handles agent path navigation using A* and transitions
// back to Idle or special states (Medic / Provider behavior).
// ============================================================
class MoveToTarget : public State {
public:
    void OnEnter(Agent* a) override;
    void Transition(Agent* a) override;
    void OnExit(Agent* a) override;
};

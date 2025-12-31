#pragma once
#include "State.h"

// ============================================================
// Idle state - default passive state for agents
// ============================================================
class Idle : public State {
public:
    void OnEnter(Agent* a) override;
    void Transition(Agent* a) override;
    void OnExit(Agent* a) override;
};

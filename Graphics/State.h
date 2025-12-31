#pragma once
class Agent;

// ============================================================
// State (abstract base class)
// Defines the interface for FSM (Finite State Machine) states.
// ============================================================
class State {
public:
    virtual ~State() = default;

    // Called once when the state becomes active
    virtual void OnEnter(Agent* a) = 0;

    // Called every update tick
    virtual void Transition(Agent* a) = 0;

    // Called before leaving the state
    virtual void OnExit(Agent* a) = 0;
};

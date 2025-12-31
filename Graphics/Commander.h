#pragma once
#include "Agent.h"
#include "Order.h"
#include <deque>
#include <vector>

class Map;
class SafetyMap;

// ------------------------------------------------------------
// Commander class - central AI controller per team
// ------------------------------------------------------------
class Commander : public Agent {
public:
    Commander(TeamColor t, int r, int c);

    // --- Rendering & Identity ---
    void render() const override;
    const char* roleLetter() const override;

    // --- Updates ---
    void update(Map& world) override;
    void updateCommanderLogic();
    void dispatchOrders(std::vector<Agent*>& team);

    // --- Orders API ---
    void addOrder(const Order& o);
    void addPriorityOrder(const Order& o);
    bool hasPendingHeal() const;
    Order nextOrder();

    // --- Combined team visibility management ---
    void setCombinedVisibility(const std::vector<uint8_t>& vis) { combinedVis = vis; }
    const std::vector<uint8_t>& getCombinedVisibility() const { return combinedVis; }

private:
    // --- Internal logic helpers ---
    void updateCombinedVisibility(const std::vector<Agent*>& team);
    void issueSupportOrders(std::vector<Agent*>& team);
    void relocateIfInDanger(Map& world, const std::vector<Agent*>& enemies);

private:
    std::deque<Order> orders;          // command queue
    std::vector<uint8_t> combinedVis;  // merged visibility from all teammates
};

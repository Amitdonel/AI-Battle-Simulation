#pragma once

// ============================================================
// Order.h
// Defines commander-issued order types and basic order struct
// ============================================================

// --- Commander order types ---
enum class OrderType {
    NONE = 0,
    ATTACK,
    DEFEND,
    HEAL,
    RESUPPLY,
    MOVE
};

// --- Basic order structure ---
struct Order {
    OrderType type = OrderType::NONE; // type of order
    int targetRow = -1;               // target row on map
    int targetCol = -1;               // target column on map

    Order() = default;

    Order(OrderType t, int r, int c)
        : type(t), targetRow(r), targetCol(c) {
    }
};

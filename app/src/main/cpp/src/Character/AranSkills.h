// This file is part of the continued Journey MMORPG client (AGPL-3.0-or-later).
#pragma once
#include <cstdint>

namespace ms::aran {
// Hidden attack variants inherit the level of the learned upgrade, never SP.
constexpr int32_t parent(int32_t id) {
    switch (id) {
        case 21110007: case 21110008: return 21110002;
        case 21120009: case 21120010: return 21120002;
        default: return id;
    }
}
constexpr bool hidden(int32_t id) { return parent(id) != id; }
constexpr bool attack(int32_t id) {
    switch (id) {
        case 21000002: case 21100001: case 21100002: case 21100004:
        case 21110003: case 21110004: case 21110006:
        case 21110007: case 21110008: case 21120005: case 21120006:
        case 21120009: case 21120010: return true;
        default: return false;
    }
}
constexpr bool command(int32_t id) {
    return attack(id) || id == 21100005 || id == 21120007;
}
constexpr int combo_cost(int32_t id) {
    switch (id) {
        case 21100004: case 21100005: return 30;
        case 21110004: return 100;
        case 21120006: case 21120007: return 200;
        default: return 0;
    }
}
constexpr int32_t swing(int stage, int full, int over) {
    if (stage == 2) return over > 0 ? 21120009 : full > 0 ? 21110007 : 21000002;
    if (stage == 3) return over > 0 ? 21120010 : full > 0 ? 21110008 : 21100001;
    return 0;
}
} // namespace ms::aran

#pragma once

#include <array>

#include <QString>

#include "stats.hpp"

struct StatUpgradeUiEntry {
    int statIndex = 0;          // Stats::StatIndex
    const char* name = "";      // Canonical tankdef/stat name
    const char* paletteKey = ""; // Settings::palette key
};

// UI display order (inverted): Health Regen first, Movement Speed last.
inline constexpr std::array<StatUpgradeUiEntry, Stats::COUNT> kStatUpgradeUi = { {
    { Stats::HEALTH_REGEN, "Health Regen", "HEALTH_REGEN" },
    { Stats::MAX_HEALTH, "Max Health", "MAX_HEALTH" },
    { Stats::BODY_DAMAGE, "Body Damage", "BODY_DAMAGE" },
    { Stats::BULLET_SPEED, "Bullet Speed", "BULLET_SPEED" },
    { Stats::BULLET_PENETRATION, "Bullet Penetration", "BULLET_PENETRATION" },
    { Stats::BULLET_DAMAGE, "Bullet Damage", "BULLET_DAMAGE" },
    { Stats::RELOAD, "Reload", "RELOAD" },
    { Stats::MOVEMENT_SPEED, "Movement Speed", "MOVEMENT_SPEED" },
} };

// Index -> canonical stat name (for looking up caps in TankDefinition.stats).
inline constexpr std::array<const char*, Stats::COUNT> kStatNamesByIndex = { {
    "Movement Speed",
    "Reload",
    "Bullet Damage",
    "Bullet Penetration",
    "Bullet Speed",
    "Body Damage",
    "Max Health",
    "Health Regen",
} };

inline constexpr const char* statNameByIndex(int statIndex) {
    if (statIndex < 0 || statIndex >= Stats::COUNT)
        return "";
    return kStatNamesByIndex[static_cast<size_t>(statIndex)];
}


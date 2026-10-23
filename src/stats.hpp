#pragma once

#include <array>
#include <algorithm>

class Stats {
public:
    enum StatIndex {
        MOVEMENT_SPEED = 0,
        RELOAD = 1,
        BULLET_DAMAGE = 2,
        BULLET_PENETRATION = 3,
        BULLET_SPEED = 4,
        BODY_DAMAGE = 5,
        MAX_HEALTH = 6,
        HEALTH_REGEN = 7,
        COUNT = 8
    };

    Stats() {
        m_stats.fill(0);
    }

    void setStat(int stat, int value) {
        if (stat < 0 || stat >= COUNT)
            return;
        m_stats[static_cast<size_t>(stat)] = std::max(0, value);
    }

    int getStat(int stat) const {
        if (stat < 0 || stat >= COUNT)
            return 0;
        return m_stats[static_cast<size_t>(stat)];
    }

    static int getStatCount(int level) {
        if (level <= 0)
            return 0;
        if (level <= 28)
            return level - 1;
        return static_cast<int>(level / 3.0f) + 18;
    }

private:
    std::array<int, COUNT> m_stats;
};


#include "tank.hpp"

#include <algorithm>

#include "game_manager.hpp"
#include "bullet.hpp"
#include "score_handler.hpp"
#include "stat_upgrade_catalog.hpp"

void Tank::updateVictim(GameObject& victim) {
    addScore(victim.getScoreReward());
    victim.killerGroup = group;
    updateLevel();
}

void Tank::updateLevel() {
    const int newLevel = ScoreHandler::scoreToLevel(score);
    if (newLevel != level) {
        level = newLevel;
        updateStats();
    }
}

bool Tank::incrementStat(int statEnum) {
    if (usedStatPoints >= maxStatPoints)
        return false;
    if (!m_def)
        return false;
    if (statEnum < 0 || statEnum >= Stats::COUNT)
        return false;

    const int cap = getStatCap(statEnum);

    const int cur = stats.getStat(statEnum);
    if (cur >= cap)
        return false;

    stats.setStat(statEnum, cur + 1);
    usedStatPoints++;
    updateStats();
    return true;
}

int Tank::getStatCap(int statEnum) const {
    if (!m_def)
        return 7;
    if (statEnum < 0 || statEnum >= Stats::COUNT)
        return 7;

    int cap = 0;
    for (const auto& s : m_def->stats) {
        if (s.name == QLatin1StringView(statNameByIndex(statEnum))) {
            cap = s.max;
            break;
        }
    }
    return (cap > 0) ? cap : 7;
}

void Tank::update(float deltaTime) {
    GameObject::update(deltaTime);

    if (!isDead())
        aliveSeconds += std::max(0.0f, deltaTime);

    if (maxHealth > 0.0f) {
        health = std::min(maxHealth, health + regenPerSecond * std::max(0.0f, deltaTime));
    }

    if (m_build) {
        m_build->update(deltaTime);

        if (m_gameManager) {
            const auto spawns = m_build->updateFire(firing, deltaTime);
            for (const auto& req : spawns) {
                m_gameManager->spawnBullet(req, *this);
            }
        }
    }
}

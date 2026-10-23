#include "fire_manager.hpp"

#include <algorithm>
#include <cmath>

#include "tank.hpp"
#include "stats.hpp"

namespace {

float baseReloadSeconds() {
    return 0.6f;
}

} // namespace

FireManager::FireManager(std::vector<FireTurretConfig> turrets)
    : m_turrets(std::move(turrets)), m_timeUntilReady(m_turrets.size(), 0.0f) {}

void FireManager::setHost(Tank* host) {
    m_host = host;
    resetDelay();
}

void FireManager::resetDelay() {
    m_paused = false;
    for (size_t i = 0; i < m_turrets.size(); i++) {
        m_timeUntilReady[i] = reloadSeconds(static_cast<int>(i)) * m_turrets[i].delayFactor;
    }
}

void FireManager::setManualReloadSeconds(std::optional<float> seconds) {
    m_manualReloadSeconds = seconds;
}

void FireManager::setFiring(bool isFiring) {
    m_isFiring = isFiring;
    if (isFiring)
        m_paused = false;
}

float FireManager::reloadSeconds(int turretIndex) const {
    const float factor = (turretIndex >= 0 && turretIndex < static_cast<int>(m_turrets.size()))
        ? m_turrets[static_cast<size_t>(turretIndex)].reloadFactor
        : 1.0f;

    if (m_manualReloadSeconds)
        return std::max(0.0f, (*m_manualReloadSeconds) * factor);

    const int reloadStat = (m_host ? m_host->getStat(Stats::RELOAD) : 0);
    const float statScale = std::pow(0.914f, static_cast<float>(reloadStat));
    return std::max(0.0f, baseReloadSeconds() * statScale * factor);
}

std::vector<int> FireManager::consumeFireIndices(float deltaTimeSeconds) {
    std::vector<int> ret;
    if (m_turrets.empty())
        return ret;

    deltaTimeSeconds = std::max(0.0f, deltaTimeSeconds);

    // Non-drone barrels: can pause when not firing.
    if (!m_paused) {
        for (size_t i = 0; i < m_turrets.size(); i++) {
            if (m_turrets[i].isDroneBarrel)
                continue;

            m_timeUntilReady[i] = std::max(0.0f, m_timeUntilReady[i] - deltaTimeSeconds);
            if (m_timeUntilReady[i] <= 0.0f) {
                if (m_isFiring) {
                    ret.push_back(static_cast<int>(i));
                    m_timeUntilReady[i] = reloadSeconds(static_cast<int>(i));
                } else {
                    // Pause everything starting next tick.
                    m_paused = true;
                }
            }
        }
    }

    // Drone barrels: always count down and fire regardless.
    for (size_t i = 0; i < m_turrets.size(); i++) {
        if (!m_turrets[i].isDroneBarrel)
            continue;

        m_timeUntilReady[i] = std::max(0.0f, m_timeUntilReady[i] - deltaTimeSeconds);
        if (m_timeUntilReady[i] <= 0.0f) {
            ret.push_back(static_cast<int>(i));
            m_timeUntilReady[i] = reloadSeconds(static_cast<int>(i));
        }
    }

    return ret;
}


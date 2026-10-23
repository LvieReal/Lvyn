#pragma once

#include <optional>
#include <vector>

class Tank;

struct FireTurretConfig {
    float delayFactor = 0.0f;
    float reloadFactor = 1.0f;
    bool isDroneBarrel = false;
};

class FireManager {
public:
    explicit FireManager(std::vector<FireTurretConfig> turrets = {});

    void setHost(Tank* host);

    void resetDelay();

    void setManualReloadSeconds(std::optional<float> seconds);

    void setFiring(bool isFiring);

    std::vector<int> consumeFireIndices(float deltaTimeSeconds);

private:
    float reloadSeconds(int turretIndex) const;

private:
    Tank* m_host = nullptr;
    std::vector<FireTurretConfig> m_turrets;
    std::vector<float> m_timeUntilReady;

    bool m_paused = false;
    bool m_isFiring = false;
    std::optional<float> m_manualReloadSeconds;
};


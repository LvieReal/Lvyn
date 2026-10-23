#pragma once

#include <optional>

#include <QPointF>

#include "game_object.hpp"

class Tank;

// Minimal port of Diep.Java Projectile:
// - Has a host tank (for group + victim credit)
// - Deletes itself when host is dead
// - Time-based lifetime (seconds, not frames)
class Projectile : public GameObject {
public:
    explicit Projectile(const Tank& host);

    const Tank* host() const {
        return m_host;
    }

    void setLifeSeconds(float seconds) {
        m_lifeSeconds = std::max(0.0f, seconds);
    }

    void update(float deltaTime) override;
    void updateVictim(GameObject& victim) override;

protected:
    const Tank* m_host = nullptr; // non-owning
    float m_ageSeconds = 0.0f;
    float m_lifeSeconds = 1.0f;
};


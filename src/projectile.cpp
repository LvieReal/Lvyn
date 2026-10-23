#include "projectile.hpp"

#include <algorithm>

#include "tank.hpp"

Projectile::Projectile(const Tank& host) {
    m_host = &host;
    group = host.group;
    noInternalCollision = true;
}

void Projectile::update(float deltaTime) {
    m_ageSeconds += std::max(0.0f, deltaTime);

    if (m_host) {
        // Match Diep.Java Projectile: delete projectile if the host is dead.
        if (m_host->isDead() || m_host->pendingDelete) {
            triggerDelete();
        }
    }

    if (m_lifeSeconds > 0.0f && m_ageSeconds >= m_lifeSeconds) {
        triggerDelete();
    }

    GameObject::update(deltaTime);
}

void Projectile::updateVictim(GameObject& victim) {
    if (m_host) {
        const_cast<Tank*>(m_host)->updateVictim(victim);
    }
}


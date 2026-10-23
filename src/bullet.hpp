#pragma once

#include <QString>

#include "projectile.hpp"
#include "tank_definitions.hpp"

class Tank;

class Bullet final : public Projectile {
public:
    Bullet(const TankBulletDefinition& def, const Tank& owner, const QPointF& spawnPos, float directionRadians, float barrelWidthWorld);

    void update(float deltaTime) override;
};

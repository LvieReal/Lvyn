#include "bullet.hpp"

#include <algorithm>
#include <cmath>

#include "tank.hpp"
#include "stats.hpp"
#include "settings.hpp"

Bullet::Bullet(const TankBulletDefinition& def, const Tank& owner, const QPointF& spawnPos, float directionRadians, float barrelWidthWorld)
    : Projectile(owner) {
    name = "bullet";
    display = "Bullet";

    geometry = Geometry::Circle;
    position = spawnPos;

    radius = std::max(1.0f, 0.5f * barrelWidthWorld * def.sizeRatio);

    const int dmgStat = owner.getStat(Stats::BULLET_DAMAGE);
    const int penStat = owner.getStat(Stats::BULLET_PENETRATION);
    const int spdStat = owner.getStat(Stats::BULLET_SPEED);

    absorptionFactor = def.absorptionFactor;
    pushFactor = 0.8f * (7.0f / 3.0f + static_cast<float>(dmgStat)) * def.damage * def.absorptionFactor;

    damage = (7.0f + 3.0f * static_cast<float>(dmgStat)) * def.damage;

    maxHealth = (8.0f + 6.0f * static_cast<float>(penStat)) * def.health;
    maxHealth = std::max(0.001f, maxHealth);
    health = maxHealth;

    noInternalCollision = true;
    keepInArena = false;
    group = owner.group;

    fillColor = owner.fillColor;
    strokeColor = Settings::globalStrokeColor;

    constexpr float kBaseLifeSeconds = 2.88f;
    setLifeSeconds(std::max(0.05f, def.lifeLength * kBaseLifeSeconds));

    const float maxSpeedPerSecond = (2.0f + 0.3f * static_cast<float>(spdStat)) * def.speed;
    velocity = QPointF(std::cos(directionRadians), std::sin(directionRadians)) * maxSpeedPerSecond;

    friction = 1.0f;
}

void Bullet::update(float deltaTime) {
    Projectile::update(deltaTime);
}

#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>

#include <QPainter>
#include <QRectF>

#include "game_object.hpp"
#include "stats.hpp"
#include "tank_build.hpp"
#include "tank_definitions.hpp"
#include "settings.hpp"

class GameManager;

class Tank : public GameObject {
public:
    Tank() : GameObject() {
        radius = 50.0f;
        keepInArena = true;
        spawnProtection = true;

        absorptionFactor = 1.0f;
        pushFactor = 8.0f;
    }

    int level = 1;
    float score = 0.0f;
    float aliveSeconds = 0.0f;

    float directionRadians = 0.0f;
    bool firing = false;

    int usedStatPoints = 0;
    int maxStatPoints = 0;

    float baseAcceleration = 0.0f;
    float regenPerSecond = 0.0f;

    void setGameManager(GameManager* world) {
        m_gameManager = world;
    }

    void applyTankDefinition(const TankDefinition& def) {
        m_def = def;
        absorptionFactor = def.absorptionFactor;
        m_speedFactor = def.speed;

        m_build = std::make_unique<TankBuild>(def);
        m_build->setHost(this);

        initHealthBar();

        updateStats();
    }

    int getStat(int statEnum) const {
        return stats.getStat(statEnum);
    }

    void setFiring(bool isFiring) {
        firing = isFiring;
    }

    float getScoreReward() const override {
        return score;
    }

    void updateVictim(GameObject& victim) override;

    void addScore(float delta) {
        score += std::max(0.0f, delta);
    }

    void updateLevel();

    bool incrementStat(int statEnum);

    int getStatCap(int statEnum) const;

    QString buildName() const {
        return m_build ? m_build->name() : QString();
    }

    float getZoom() const {
        const float field = m_build ? m_build->fieldFactor() : 1.0f;
        return (0.55f * field) / scale;
    }

    QRectF getView(const QSizeF& viewportSize) const {
        const float zoom = std::max(0.001f, getZoom());
        const float w = viewportSize.width() / zoom;
        const float h = viewportSize.height() / zoom;
        return QRectF(position.x() - w * 0.5f, position.y() - h * 0.5f, w, h);
    }

    void updateStats() {
        const float prevMax = maxHealth;

        const float baseMax = m_def ? m_def->maxHealth : 50.0f;
        const float newMax = baseMax + (2.0f * static_cast<float>(level - 1)) + (20.0f * static_cast<float>(getStat(Stats::MAX_HEALTH)));
        maxHealth = std::max(0.0f, newMax);

        if (prevMax > 0.0f) {
            // Preserve health percentage when max health changes.
            health = health * (maxHealth / prevMax);
        } else {
            health = maxHealth;
        }
        health = std::clamp(health, 0.0f, maxHealth);

        damage = 20.0f + 6.0f * static_cast<float>(getStat(Stats::BODY_DAMAGE));

        const float cv = static_cast<float>(2.55 * std::pow(1.07, static_cast<float>(getStat(Stats::MOVEMENT_SPEED))) / std::pow(1.015, static_cast<float>(level - 1)));
        baseAcceleration = cv * (1.0f - friction);
        baseAcceleration *= m_speedFactor;

        regenPerSecond = (maxHealth + 4.0f * maxHealth * static_cast<float>(getStat(Stats::HEALTH_REGEN))) / 1000.0f;

        scale = std::pow(1.01f, static_cast<float>(level - 1)) / std::pow(1.01f, (45 - 1));

        maxStatPoints = Stats::getStatCount(level);
    }

    void update(float deltaTime) override;

    void draw(QPainter* painter) const override {
        if (m_build)
            m_build->draw(painter);

        GameObject::draw(painter);
    }

public:
    Stats stats;

private:
    GameManager* m_gameManager = nullptr;
    std::optional<TankDefinition> m_def;
    float m_speedFactor = 1.0f;
    std::unique_ptr<TankBuild> m_build;
};

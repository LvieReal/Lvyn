#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>
#include <limits>

#include <QPointF>
#include <QRandomGenerator>

#include "game_object.hpp"
#include "polygon_definitions.hpp"
#include "settings.hpp"

class Polygon final : public GameObject {
public:
    Polygon(const PolygonDefinition& def, const QPointF& spawnPos, bool isNestPolygon)
        : GameObject(), m_defId(def.id), m_isNestPolygon(isNestPolygon), m_scoreReward(def.scoreReward) {
        name = def.id;
        display = def.display;
        zIndex = def.zIndex;

        geometry = Geometry::Polygon;
        radius = def.radius;
        scale = 1.0f;

        fillColor = def.fillColor;

        maxHealth = def.maxHealth;
        health = def.maxHealth;
        damage = def.damage;

        friction = def.friction;
        absorptionFactor = def.absorptionFactor;
        pushFactor = def.pushFactor;

        keepInArena = true;

        group = kPolygonGroup;

        initHealthBar();

        position = spawnPos;

        localPolygon = def.localPolygon.isEmpty() ? makeRegularPolygon(def.sides, def.radius) : def.localPolygon;

        const float signOrbit = (rand01() < 0.5f) ? 1.0f : -1.0f;
        const float signRot = (rand01() < 0.5f) ? 1.0f : -1.0f;

        const float orbitRand = randRange(def.orbitRateRandomMin, def.orbitRateRandomMax);
        m_orbitRate = signOrbit * def.orbitRate * orbitRand;
        m_rotationRate = signRot * def.rotationRate;
        m_baseAcceleration = def.baseAcceleration;
        m_edgeBuffer = def.edgeBuffer;

        rotationRadians = randRange(0.0f, std::numbers::pi_v<float>);
        m_directionRadians = randRange(0.0f, std::numbers::pi_v<float> * 2.0f);
    }

    void update(float deltaTime) override {
        GameObject::update(deltaTime);

        rotationRadians += m_rotationRate;
        m_directionRadians += m_orbitRate;

        if (m_edgeBuffer > 0.0f) {
            const float left = m_edgeBuffer;
            const float right = static_cast<float>(Settings::arenaWidth) - m_edgeBuffer;
            const float top = m_edgeBuffer;
            const float bottom = static_cast<float>(Settings::arenaHeight) - m_edgeBuffer;

            if (position.x() < left)
                m_directionRadians = 0.0f;
            else if (position.x() > right)
                m_directionRadians = std::numbers::pi_v<float>;
            else if (position.y() < top)
                m_directionRadians = std::numbers::pi_v<float> * 0.5f;
            else if (position.y() > bottom)
                m_directionRadians = std::numbers::pi_v<float> * 1.5f;
        }

        addForce(m_baseAcceleration * deltaTime, m_directionRadians);
    }

    bool isNestPolygon() const {
        return m_isNestPolygon;
    }

    // Parsed but unused in this project so far (see PolygonDefinition::scoreReward).
    float scoreReward() const {
        return m_scoreReward;
    }

    float getScoreReward() const override {
        return m_scoreReward;
    }

private:
    static float rand01() {
        return QRandomGenerator::global()->generateDouble();
    }

    static float randRange(float minV, float maxV) {
        if (maxV < minV)
            std::swap(minV, maxV);
        const float t = rand01();
        return (1.0f - t) * minV + t * maxV;
    }

    static QPolygonF makeRegularPolygon(int sides, float radius) {
        QPolygonF poly;
        poly.reserve(sides);
        for (int i = 0; i < sides; i++) {
            const float t = static_cast<float>(i) / static_cast<float>(sides);
            const float a = t * std::numbers::pi_v<float> * 2.0f;
            poly.push_back(QPointF(std::cos(a) * radius, std::sin(a) * radius));
        }
        return poly;
    }

private:
    static inline constexpr int kPolygonGroup = std::numeric_limits<int>::min();

    QString m_defId;
    bool m_isNestPolygon = false;

    float m_scoreReward = 0.0f;

    float m_directionRadians = 0.0f;
    float m_baseAcceleration = 0.0f;
    float m_orbitRate = 0.0f;
    float m_rotationRate = 0.0f;
    float m_edgeBuffer = 0.0f;
};

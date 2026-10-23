#include <algorithm>
#include <cmath>
#include <numbers>

#include <QColor>
#include <QPainter>
#include <QPointF>
#include <QRandomGenerator>

#include "settings.hpp"
#include "tank_build.hpp"
#include "tank.hpp"

namespace {

QPointF rotatePoint(const QPointF& p, float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return QPointF(p.x() * c - p.y() * s, p.x() * s + p.y() * c);
}

float randRange(float minV, float maxV) {
    if (maxV < minV)
        std::swap(minV, maxV);
    const float t = static_cast<float>(QRandomGenerator::global()->generateDouble());
    return (1.0f - t) * minV + t * maxV;
}

float clamp01(float x) {
    return std::clamp(x, 0.0f, 1.0f);
}

float recoilDistFrom(float recoil) {
    const float recoilDistFactor = static_cast<float>(9.51697 * std::pow(0.767725, static_cast<double>(recoil)) + 0.693587);
    return std::max(4.5f, 0.6f * recoilDistFactor * recoil);
}

float recoilTimeSecondsFromReloadFactor(float reloadFactor) {
    const int recoilTimeFrames = std::max(15, static_cast<int>(std::lround(-44.9793 * std::pow(0.111316, static_cast<double>(reloadFactor)) + 35.0069)));

    constexpr float kFramesPerSecond = 120.0f;
    return recoilTimeFrames / kFramesPerSecond;
}

} // namespace

TankBuild::TankBuild(const TankDefinition& def)
    : m_name(def.name),
      m_fieldFactor(def.fieldFactor),
      m_zoomAbility(def.flags.zoomAbility) {
    m_barrels.reserve(def.barrels.size());

    std::vector<FireTurretConfig> turrets;
    turrets.reserve(def.barrels.size());

    for (const auto& b : def.barrels) {
        BarrelRuntime rt;
        rt.def = b;
        rt.recoilDist = recoilDistFrom(b.recoil);
        rt.recoilDurationSeconds = std::max(0.0f, recoilTimeSecondsFromReloadFactor(b.reloadFactor));
        rt.recoilElapsedSeconds = rt.recoilDurationSeconds;
        rt.recoilActive = false;
        rt.recoilForceApplied = true;
        m_barrels.push_back(rt);

        FireTurretConfig t;
        t.delayFactor = b.delayFactor;
        t.reloadFactor = b.reloadFactor;
        t.isDroneBarrel = (b.bullet.type == "drone" || b.bullet.type == "swarm");
        turrets.push_back(t);
    }

    m_fireManager = FireManager(std::move(turrets));
}

void TankBuild::setHost(Tank* host) {
    m_host = host;
    m_fireManager.setHost(host);
    updateBarrelTransforms();
}

void TankBuild::update(float deltaTimeSeconds) {
    updateBarrelTransforms();

    if (!m_host)
        return;

    const float dt = std::max(0.0f, deltaTimeSeconds);
    for (auto& b : m_barrels) {
        if (!b.recoilActive || b.recoilDurationSeconds <= 0.0f)
            continue;

        b.recoilElapsedSeconds = std::min(b.recoilDurationSeconds, b.recoilElapsedSeconds + dt);

        if (!b.recoilForceApplied && b.recoilElapsedSeconds >= (b.recoilDurationSeconds * 0.5f)) {
            b.recoilForceApplied = true;
            m_host->addForceVec(b.pendingRecoilForce);
        }

        if (b.recoilElapsedSeconds >= b.recoilDurationSeconds) {
            b.recoilActive = false;
        }
    }
}

void TankBuild::updateBarrelTransforms() {
    if (!m_host)
        return;

    const float tankAngle = m_host->directionRadians;
    const float s = m_host->scale;

    for (auto& b : m_barrels) {
        // Rotate (0, offset) by barrel angle (relative), then rotate by tank angle.
        const QPointF rel = rotatePoint(QPointF(0.0f, b.def.offset) * s, static_cast<float>(b.def.angleRadians));
        const QPointF worldOff = rotatePoint(rel, tankAngle);
        b.basePosWorld = m_host->position + worldOff;
        b.angleWorldRadians = tankAngle + static_cast<float>(b.def.angleRadians);
    }
}

void TankBuild::startRecoil(int barrelIndex, float finalAngleRadians) {
    if (!m_host)
        return;
    if (barrelIndex < 0 || barrelIndex >= static_cast<int>(m_barrels.size()))
        return;

    auto& b = m_barrels[static_cast<size_t>(barrelIndex)];
    if (b.recoilDurationSeconds <= 0.0f) {
        b.recoilActive = false;
        b.recoilForceApplied = true;
        return;
    }

    b.recoilActive = true;
    b.recoilElapsedSeconds = 0.0f;
    b.recoilForceApplied = false;

    constexpr float kBaseFriction = 0.9f;
    const float frictionScale = (1.0f - m_host->friction) / std::max(1e-6f, (1.0f - kBaseFriction));
    const float recoilMagnitude = 2.0f * b.def.recoil * frictionScale;

    const QPointF recoilDir(-std::cos(finalAngleRadians), -std::sin(finalAngleRadians));
    b.pendingRecoilForce = recoilDir * recoilMagnitude;
}

float TankBuild::lengthShift(const BarrelRuntime& b) const {
    if (!m_host || !b.recoilActive || b.recoilDurationSeconds <= 0.0f)
        return 0.0f;

    const float turretLengthScaled = b.def.size * m_host->scale;
    const float recoilDist = std::min(turretLengthScaled * 0.07f, b.recoilDist);

    const float t = clamp01(b.recoilElapsedSeconds / b.recoilDurationSeconds);

    return -std::abs(recoilDist) * std::cos(std::numbers::pi_v<float> * (t - 0.5f));
}

std::vector<BulletSpawnRequest> TankBuild::updateFire(bool isFiring, float deltaTimeSeconds) {
    std::vector<BulletSpawnRequest> spawns;
    if (!m_host)
        return spawns;

    m_fireManager.setFiring(isFiring);
    const auto indices = m_fireManager.consumeFireIndices(deltaTimeSeconds);
    if (indices.empty())
        return spawns;

    spawns.reserve(indices.size());

    // Scatter is randomized per shot.
    constexpr float kScatterDegreesMin = -5.0f;
    constexpr float kScatterDegreesMax = 5.0f;
    const float degToRad = std::numbers::pi_v<float> / 180.0f;

    for (const int idx : indices) {
        if (idx < 0 || idx >= static_cast<int>(m_barrels.size()))
            continue;

        const auto& b = m_barrels[static_cast<size_t>(idx)];

        // Only implement basic bullet type for now.
        if (b.def.bullet.type != "bullet")
            continue;

        const float length = b.def.size * m_host->scale;
        const float width = b.def.width * m_host->scale;
        const QPointF forward(std::cos(b.angleWorldRadians), std::sin(b.angleWorldRadians));

        const float scatterDegrees = randRange(kScatterDegreesMin, kScatterDegreesMax) * b.def.bullet.scatterRate;
        const float scatterRad = scatterDegrees * degToRad;
        const float finalAngle = b.angleWorldRadians + scatterRad;

        startRecoil(idx, finalAngle);

        BulletSpawnRequest req;
        req.position = b.basePosWorld + forward * length;
        req.directionRadians = finalAngle;
        req.barrelWidth = width;
        req.bullet = b.def.bullet;
        spawns.push_back(req);
    }

    return spawns;
}

void TankBuild::draw(QPainter* painter) const {
    if (!m_host || !painter)
        return;

    // Simple turret drawing (rectangular barrels).
    painter->save();
    painter->setOpacity(m_host->opacity * m_host->opacity);

    painter->setBrush(m_host->getDamageLerpColor(QColor(160, 160, 160)));
    painter->setPen(QPen(m_host->getDamageLerpColor(Settings::globalStrokeColor), 5));

    for (const auto& b : m_barrels) {
        const float length = std::max(0.0f, (b.def.size + lengthShift(b)) * m_host->scale);
        const float width = b.def.width * m_host->scale;

        painter->save();
        painter->translate(b.basePosWorld);
        painter->rotate(b.angleWorldRadians * 180.0f / std::numbers::pi_v<float>);

        // Draw barrel from origin forward (x axis).
        painter->drawRect(QRectF(0.0, -width * 0.5, length, width));
        painter->restore();
    }

    painter->restore();
}

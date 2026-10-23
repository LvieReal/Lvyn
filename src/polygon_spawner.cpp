#include "polygon_spawner.hpp"

#include <algorithm>
#include <cmath>

#include <QRandomGenerator>

#include "game_manager.hpp"
#include "player.hpp"
#include "polygon.hpp"
#include "settings.hpp"

namespace {

double rand01() {
    return QRandomGenerator::global()->generateDouble();
}

QString pickWeighted(const QList<WeightedPolygonId>& weights) {
    double total = 0.0;
    for (const auto& w : weights)
        total += std::max(0.0, w.weight);

    if (total <= 0.0)
        return QString();

    const double r = rand01() * total;
    double acc = 0.0;
    for (const auto& w : weights) {
        acc += std::max(0.0, w.weight);
        if (r <= acc)
            return w.id;
    }
    return weights.back().id;
}

} // namespace

PolygonSpawner::PolygonSpawner(GameManager* gameManager) : m_gameManager(gameManager) {}

bool PolygonSpawner::setDefinitions(const PolygonDefinitions* defs, QString* outError) {
    m_defs = defs;
    if (!m_defs) {
        if (outError)
            *outError = "PolygonSpawner: definitions pointer is null";
        return false;
    }
    if (m_defs->spawner.randomPolygonWeights.isEmpty()) {
        if (outError)
            *outError = "PolygonSpawner: spawner.randomPolygonWeights is empty";
        return false;
    }

    // Ensure required polygon IDs exist.
    for (const auto& w : m_defs->spawner.randomPolygonWeights) {
        if (!m_defs->find(w.id)) {
            if (outError)
                *outError = QString("PolygonSpawner: randomPolygonWeights id '%1' not found in polygons[]").arg(w.id);
            return false;
        }
    }
    if (!m_defs->find(m_defs->spawner.nestPolygonId)) {
        if (outError)
            *outError = QString("PolygonSpawner: required polygon id '%1' not found").arg(m_defs->spawner.nestPolygonId);
        return false;
    }
    if (!m_defs->find(m_defs->spawner.alphaNestPolygonId)) {
        if (outError)
            *outError = QString("PolygonSpawner: required polygon id '%1' not found").arg(m_defs->spawner.alphaNestPolygonId);
        return false;
    }

    return true;
}

void PolygonSpawner::reset() {
    if (!m_defs)
        return;

    const float arenaW = static_cast<float>(Settings::arenaWidth);
    const float arenaH = static_cast<float>(Settings::arenaHeight);
    const float grid = static_cast<float>(Settings::arenaGridSize);

    const float arenaSide = std::min(arenaW, arenaH);
    const float nestSide = arenaSide / std::max(0.001f, static_cast<float>(m_defs->spawner.nestSideDivisor));
    const float crasherSide = nestSide * static_cast<float>(m_defs->spawner.crasherSideMultiplier);

    m_nestBox = QRectF(arenaW * 0.5f - nestSide * 0.5f, arenaH * 0.5f - nestSide * 0.5f, nestSide, nestSide);
    m_crasherZone = QRectF(arenaW * 0.5f - crasherSide * 0.5f, arenaH * 0.5f - crasherSide * 0.5f, crasherSide, crasherSide);

    const float arenaArea = arenaW * arenaH;
    const float polygonAreaNorm = static_cast<float>(m_defs->spawner.polygonAreaDivisor) * grid * grid;
    const float nestAreaNorm = static_cast<float>(m_defs->spawner.nestAreaDivisor) * grid * grid;

    m_polygonAmount = (polygonAreaNorm > 0.0f) ? static_cast<int>(arenaArea / polygonAreaNorm) : 0;
    const float nestArea = nestSide * nestSide;
    // Use ceil so typical configurations don't floor to 1 polygon forever.
    m_pentagonNestAmount =
        (nestAreaNorm > 0.0f) ? std::max(1, static_cast<int>(std::ceil(static_cast<double>(nestArea / nestAreaNorm)))) : 0;

    m_count = 0;
    m_nestCount = 0;
    m_alphaCount = 0;
}

void PolygonSpawner::update() {
    if (!m_defs || !m_gameManager)
        return;

    recount();

    while (m_count < m_polygonAmount)
        spawnRandomPolygon();

    while (m_nestCount < m_pentagonNestAmount)
        spawnNestPolygon();

    // NOTE: Crashers intentionally not implemented yet.
}

void PolygonSpawner::recount() {
    m_count = 0;
    m_nestCount = 0;
    m_alphaCount = 0;

    for (const auto& [id, obj] : m_gameManager->getObjects()) {
        (void)id;
        auto* poly = dynamic_cast<Polygon*>(obj);
        if (!poly || poly->pendingDelete)
            continue;

        if (poly->isNestPolygon()) {
            m_nestCount++;
            if (poly->name == m_defs->spawner.alphaNestPolygonId)
                m_alphaCount++;
        } else {
            m_count++;
        }
    }
}

QPointF PolygonSpawner::randomPointInRect(const QRectF& r) const {
    const double x = r.x() + rand01() * r.width();
    const double y = r.y() + rand01() * r.height();
    return QPointF(x, y);
}

bool PolygonSpawner::isPointInsideCrasherZone(const QPointF& p) const {
    return m_crasherZone.contains(p);
}

void PolygonSpawner::spawnRandomPolygon() {
    QPointF pos;
    do {
        pos = QPointF(rand01() * Settings::arenaWidth, rand01() * Settings::arenaHeight);
    } while (m_nestBox.contains(pos));

    const QString id = pickWeighted(m_defs->spawner.randomPolygonWeights);
    if (id.isEmpty())
        return;

    if (m_gameManager->spawnPolygon(id, pos, /*isNestPolygon=*/false))
        m_count++;
}

void PolygonSpawner::spawnNestPolygon() {
    if (!m_gameManager)
        return;

    const Player* player = m_gameManager->getLocalPlayer();
    const float avoidDist = static_cast<float>(m_defs->spawner.avoidPlayerDistance);
    const float avoidRadius = (player ? (avoidDist + player->getRadiusScaled()) : 0.0f);
    const float avoidRadiusSq = avoidRadius * avoidRadius;

    QPointF pos;
    bool onPlayer = false;
    do {
        pos = randomPointInRect(m_nestBox);
        onPlayer = false;
        if (player) {
            const QPointF d = pos - player->position;
            if ((d.x() * d.x() + d.y() * d.y()) < avoidRadiusSq)
                onPlayer = true;
        }
    } while (onPlayer);

    bool isAlpha = rand01() < m_defs->spawner.alphaPentagonChance;
    if (m_alphaCount >= m_defs->spawner.alphaPentagonMax)
        isAlpha = false;

    const QString id = isAlpha ? m_defs->spawner.alphaNestPolygonId : m_defs->spawner.nestPolygonId;
    if (m_gameManager->spawnPolygon(id, pos, /*isNestPolygon=*/true)) {
        m_nestCount++;
        if (isAlpha)
            m_alphaCount++;
    }
}

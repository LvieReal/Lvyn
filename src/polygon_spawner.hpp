#pragma once

#include <optional>

#include <QPointF>
#include <QRectF>

#include "polygon_definitions.hpp"

class GameManager;

class PolygonSpawner {
public:
    PolygonSpawner() = default;
    explicit PolygonSpawner(GameManager* gameManager);

    void setGameManager(GameManager* gameManager) {
        m_gameManager = gameManager;
    }

    bool setDefinitions(const PolygonDefinitions* defs, QString* outError);
    void reset();
    void update();

private:
    void recount();
    void spawnRandomPolygon();
    void spawnNestPolygon();

    QPointF randomPointInRect(const QRectF& r) const;
    bool isPointInsideCrasherZone(const QPointF& p) const;

private:
    GameManager* m_gameManager = nullptr;
    const PolygonDefinitions* m_defs = nullptr;

    QRectF m_nestBox;
    QRectF m_crasherZone;

    int m_count = 0;
    int m_nestCount = 0;
    int m_alphaCount = 0;

    int m_polygonAmount = 0;
    int m_pentagonNestAmount = 0;
};

#pragma once

#include <optional>

#include <QColor>
#include <QHash>
#include <QPolygonF>
#include <QRectF>
#include <QString>

struct PolygonDefinition {
    QString id;
    QString display;

    int sides = 0;
    float radius = 0.0f;
    QPolygonF localPolygon; // If empty, a regular n-gon is generated from `sides`/`radius`.

    float zIndex = -1.0f;

    QColor fillColor;
    QColor strokeColor;

    float maxHealth = 0.0f;
    float damage = 0.0f;

    float scoreReward = 0.0f;

    float friction = 0.988f;
    float absorptionFactor = 0.0f;
    float pushFactor = 0.0f;

    float baseAcceleration = 0.0f; // per-physics-tick acceleration (see GameObject::addForce)
    float orbitRate = 0.0f;        // radians per physics tick
    float orbitRateRandomMin = 1.0f;
    float orbitRateRandomMax = 1.0f;
    float rotationRate = 0.0f;     // radians per physics tick
    float edgeBuffer = 0.0f;       // distance from arena edge to "turn around" (world units)
};

struct WeightedPolygonId {
    QString id;
    double weight = 0.0;
};

struct PolygonSpawnerConfig {
    double nestSideDivisor = 6.3;
    double crasherSideMultiplier = 2.0;

    double polygonAreaDivisor = 180.0;
    double nestAreaDivisor = 220.0;

    double avoidPlayerDistance = 173.0;

    QString nestPolygonId = "pentagon";
    QString alphaNestPolygonId = "alpha_pentagon";

    double alphaPentagonChance = 0.01;
    int alphaPentagonMax = 5;

    QList<WeightedPolygonId> randomPolygonWeights;
};

struct PolygonDefinitions {
    PolygonSpawnerConfig spawner;
    QHash<QString, PolygonDefinition> polygons;

    const PolygonDefinition* find(const QString& id) const {
        auto it = polygons.find(id);
        if (it == polygons.end())
            return nullptr;
        return &it.value();
    }
};

// Loads `config/PolygonDefinitions.json` (or any compatible file) into runtime definitions.
// On failure, returns std::nullopt and writes a message to `outError` (if non-null).
std::optional<PolygonDefinitions> loadPolygonDefinitionsFromFile(const QString& filePath, QString* outError);
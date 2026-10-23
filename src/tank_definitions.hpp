#pragma once

#include <optional>

#include <QColor>
#include <QHash>
#include <QString>

struct TankBulletDefinition {
    QString type;
    float sizeRatio = 1.0f;
    float health = 1.0f;
    float damage = 1.0f;
    float speed = 1.0f;
    float scatterRate = 1.0f;
    float lifeLength = 1.0f;
    float absorptionFactor = 1.0f;
};

struct TankBarrelDefinition {
    double angleRadians = 0.0;
    float offset = 0.0f;
    float size = 0.0f;
    float width = 0.0f;

    float delayFactor = 0.0f;
    float reloadFactor = 1.0f;
    float recoil = 1.0f;

    bool isTrapezoid = false;
    double trapezoidDirectionRadians = 0.0;

    QString addon;

    TankBulletDefinition bullet;
};

struct TankStatCap {
    QString name;
    int max = 0;
};

struct TankFlagsDefinition {
    bool invisibility = false;
    bool zoomAbility = false;
    bool canClaimSquares = false;
    bool devOnly = false;
};

struct TankDefinition {
    int id = 0;
    QString name;

    TankFlagsDefinition flags;

    float fieldFactor = 1.0f;
    float absorptionFactor = 1.0f;
    float speed = 1.0f;
    float maxHealth = 50.0f;

    float visibilityRateShooting = 0.23f;
    float visibilityRateMoving = 0.08f;
    float invisibilityRate = 0.03f;

    int sides = 1;
    float borderWidth = 15.0f;

    QList<TankBarrelDefinition> barrels;
    QList<TankStatCap> stats;
};

struct TankDefinitions {
    QHash<QString, TankDefinition> byNameLower;

    const TankDefinition* findByName(const QString& name) const {
        const QString key = name.trimmed().toLower();
        auto it = byNameLower.find(key);
        if (it == byNameLower.end())
            return nullptr;
        return &it.value();
    }
};

// Loads `config/TankDefinitions.json` into runtime definitions.
// On failure, returns std::nullopt and writes a message to `outError` (if non-null).
std::optional<TankDefinitions> loadTankDefinitionsFromFile(const QString& filePath, QString* outError);

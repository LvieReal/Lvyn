#pragma once

#include <vector>

#include <QPointF>
#include <QString>

#include "fire_manager.hpp"
#include "tank_definitions.hpp"

class QPainter;
class Tank;

struct BulletSpawnRequest {
    QPointF position;
    float directionRadians = 0.0f;
    float barrelWidth = 0.0f;
    TankBulletDefinition bullet;
};

class TankBuild {
public:
    explicit TankBuild(const TankDefinition& def);

    void setHost(Tank* host);

    const QString& name() const {
        return m_name;
    }

    float fieldFactor() const {
        return m_fieldFactor;
    }

    bool zoomAbility() const {
        return m_zoomAbility;
    }

    void update(float deltaTimeSeconds);
    std::vector<BulletSpawnRequest> updateFire(bool isFiring, float deltaTimeSeconds);

    void draw(QPainter* painter) const;

private:
    struct BarrelRuntime {
        TankBarrelDefinition def;
        QPointF basePosWorld;
        float angleWorldRadians = 0.0f;

        float recoilDist = 0.0f;
        float recoilDurationSeconds = 0.0f;
        float recoilElapsedSeconds = 0.0f;
        bool recoilActive = false;

        QPointF pendingRecoilForce;
        bool recoilForceApplied = true;
    };

    void updateBarrelTransforms();
    void startRecoil(int barrelIndex, float finalAngleRadians);
    float lengthShift(const BarrelRuntime& b) const;

private:
    QString m_name;
    float m_fieldFactor = 1.0f;
    bool m_zoomAbility = false;

    Tank* m_host = nullptr;

    std::vector<BarrelRuntime> m_barrels;
    FireManager m_fireManager;
};

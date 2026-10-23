#pragma once

#include <algorithm>

#include <QPointF>

class GameObject;

// NOTE: Currently does not implement:
// - Predator/zoom-ability camera targeting (aim-forward snap)
// - Mouse wheel zoom controls
class CameraManager {
public:
    void setHost(GameObject* host) {
        m_host = host;
        if (m_host)
            forceSnapToHost();
    }

    GameObject* host() const {
        return m_host;
    }

    void setLerpFactor(float lerpFactor) {
        m_lerpFactor = std::clamp(lerpFactor, 0.0f, 1.0f);
    }

    float lerpFactor() const {
        return m_lerpFactor;
    }

    void forceSnapToHost();

    void forceSetTarget(const QPointF& target) {
        m_currentTarget = target;
    }

    void setTargetZoom(float zoom) {
        m_targetZoom = std::max(0.001f, zoom);
    }

    void forceSetZoom(float zoom) {
        m_targetZoom = m_currentZoom = std::max(0.001f, zoom);
    }

    float zoom() const {
        return m_currentZoom;
    }

    void setZoomLerpFactor(float lerpFactor) {
        m_zoomLerpFactor = std::clamp(lerpFactor, 0.0f, 1.0f);
    }

    float zoomLerpFactor() const {
        return m_zoomLerpFactor;
    }

    void update(float deltaTime, float referenceDeltaTime);

    QPointF target() const {
        return m_currentTarget;
    }

private:
    GameObject* m_host = nullptr; // non-owning
    QPointF m_currentTarget;
    float m_lerpFactor = 0.05f;

    float m_targetZoom = 1.0f;
    float m_currentZoom = 1.0f;
    float m_zoomLerpFactor = 0.05f;
};

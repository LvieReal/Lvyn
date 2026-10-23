#include "camera_manager.hpp"

#include <cmath>

#include "game_object.hpp"

void CameraManager::forceSnapToHost() {
    if (!m_host)
        return;
    m_currentTarget = m_host->position;
}

void CameraManager::update(float deltaTime, float referenceDeltaTime) {
    if (!m_host)
        return;

    // Lerp camera zoom (CameraManager.java behavior), but make it stable across step size.
    if (std::abs(m_targetZoom - m_currentZoom) > 1e-3f) {
        const float ref = std::max(1e-6f, referenceDeltaTime);
        const float ratio = std::max(0.0f, deltaTime) / ref;
        const float t = std::clamp(1.0f - std::pow(1.0f - m_zoomLerpFactor, ratio), 0.0f, 1.0f);
        m_currentZoom += (m_targetZoom - m_currentZoom) * t;
    }

    // Make lerp factor stable across variable step sizes:
    // - At deltaTime == referenceDeltaTime -> t == lerpFactor
    // - Larger deltaTime -> t increases smoothly (still clamped to [0,1])
    const float ref = std::max(1e-6f, referenceDeltaTime);
    const float ratio = std::max(0.0f, deltaTime) / ref;
    const float t = std::clamp(1.0f - std::pow(1.0f - m_lerpFactor, ratio), 0.0f, 1.0f);

    const QPointF desired = m_host->position;
    const QPointF diff = desired - m_currentTarget;
    m_currentTarget += diff * t;
}

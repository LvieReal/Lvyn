#pragma once

#include <map>
#include <cmath>

#include <Qt>
#include <QString>
#include <QPointF>
#include <QSizeF>

#include "tank.hpp"
#include "input_state.hpp"

class PlayerController {
public:
    void setInputState(InputState* inputState) {
        m_inputState = inputState;
    }

    void setHost(Tank* newHost) {
        m_host = newHost;
    }

    void updateMovement() {
        if (!m_inputState || !m_host)
            return;

        int x = 0, y = 0;

        if (m_inputState->isKeyDown(movementKeys["Up"]))
            y--;
        if (m_inputState->isKeyDown(movementKeys["Down"]))
            y++;
        if (m_inputState->isKeyDown(movementKeys["Right"]))
            x++;
        if (m_inputState->isKeyDown(movementKeys["Left"]))
            x--;

        if (x == 0 && y == 0)
            return;

        if (m_host->spawnProtection)
            m_host->spawnProtection = false;

        m_host->addForce(m_host->baseAcceleration, std::atan2f(y, x));
    }

    void updateAimAndFire(const QPointF& viewPos, const QSizeF& viewSize, float zoom) {
        if (!m_inputState || !m_host)
            return;

        zoom = std::max(0.001f, zoom);

        // Mouse is in window coordinates; convert to world coordinates.
        const QPointF mouse = m_inputState->mousePosition;
        const QPointF worldMouse =
            viewPos
            + QPointF(mouse.x() - viewSize.width() * 0.5, mouse.y() - viewSize.height() * 0.5) / zoom;

        const QPointF d = worldMouse - m_host->position;
        m_host->directionRadians = std::atan2(static_cast<float>(d.y()), static_cast<float>(d.x()));

        const bool firing = m_inputState->isMouseDown(Qt::LeftButton);
        if (firing && m_host->spawnProtection)
            m_host->spawnProtection = false;
        m_host->setFiring(firing);
    }

private:
    std::map<QString, int> movementKeys = {
        {"Up", Qt::Key_W},
        {"Down", Qt::Key_S},
        {"Right", Qt::Key_D},
        {"Left", Qt::Key_A}
    };

    Tank* m_host = nullptr;
    InputState* m_inputState = nullptr;
};

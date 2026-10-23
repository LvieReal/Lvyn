#pragma once

#include <QPointF>
#include <QRectF>
#include <QPainter>
#include <QSizeF>

#include <vector>
#include <algorithm>

#include "settings.hpp"
#include "game_manager.hpp"
#include "game_object.hpp"

class Arena {
public:
    Arena(GameManager &gameManager) : m_gameManager(gameManager) {}

    void drawObjects(QPainter* painter, const QRectF& viewRect) const {
        std::vector<GameObject*> visible;
        visible.reserve(m_gameManager.getObjects().size());

        for (const auto& [id, obj] : m_gameManager.getObjects()) {
            (void)id;
            if (!obj || obj->isInvisible() || !viewRect.intersects(obj->getBoundingBox()))
                continue;
            visible.push_back(obj);
        }

        std::sort(visible.begin(), visible.end(), [](const GameObject* a, const GameObject* b) {
            if (a->zIndex != b->zIndex)
                return a->zIndex < b->zIndex;
            return a->getId() < b->getId();
        });

        for (const auto* obj : visible)
            obj->draw(painter);
    }

private:
    GameManager &m_gameManager;
};

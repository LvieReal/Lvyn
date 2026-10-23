#include "arena_view_item.hpp"

#include <algorithm>

#include <QPainter>

#include "game_manager.hpp"
#include "arena.hpp"

ArenaViewItem::ArenaViewItem(GameManager& gameManager, Arena* arena)
    : m_gameManager(gameManager), m_arena(arena) {
    setAntialiasing(true);
    setOpaquePainting(false);
}

void ArenaViewItem::paint(QPainter* painter) {
    if (!m_arena)
        return;

    painter->setRenderHint(QPainter::Antialiasing, true);

    const QPointF viewPos = m_gameManager.getViewPos();
    const float zoom = std::max(0.001f, m_gameManager.getViewZoom());

    const QSizeF viewportSize(width(), height());
    const QRectF viewRect(
        viewPos.x() - viewportSize.width() / (2.0f * zoom),
        viewPos.y() - viewportSize.height() / (2.0f * zoom),
        viewportSize.width() / zoom,
        viewportSize.height() / zoom
    );

    painter->save();
    painter->translate(viewportSize.width() * 0.5, viewportSize.height() * 0.5);
    painter->scale(zoom, zoom);
    painter->translate(-viewPos);

    m_arena->drawObjects(painter, viewRect);
    painter->restore();
}

#include "ui_view_item.hpp"

#include <QPainter>

#include "game_manager.hpp"

UiViewItem::UiViewItem(GameManager& gameManager) : m_gameManager(gameManager) {
    setAntialiasing(true);
    setOpaquePainting(false);
}

void UiViewItem::paint(QPainter* painter) {
    if (!painter)
        return;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    m_gameManager.drawUi(painter, QSizeF(width(), height()));
}

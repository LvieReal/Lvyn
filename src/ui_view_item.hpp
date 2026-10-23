#pragma once

#include <QQuickPaintedItem>

class GameManager;

// UI overlay item drawn in screen coordinates (not affected by camera transforms).
class UiViewItem final : public QQuickPaintedItem {
public:
    explicit UiViewItem(GameManager& gameManager);

    void paint(QPainter* painter) override;

private:
    GameManager& m_gameManager;
};


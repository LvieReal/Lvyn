#pragma once

#include <QPointF>
#include <QQuickPaintedItem>

class GameManager;
class Arena;

// QQuickPaintedItem that draws game objects using QPainter.
// This keeps rendering in C++ and avoids any QML usage.
class ArenaViewItem final : public QQuickPaintedItem {
public:
    ArenaViewItem(GameManager& gameManager, Arena* arena);

    void paint(QPainter* painter) override;

private:
    GameManager& m_gameManager;
    Arena* m_arena = nullptr; // non-owning, managed by main()
};

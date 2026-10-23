#pragma once

#include <functional>

#include <QColor>
#include <QElapsedTimer>
#include <QString>

#include "ui_object.hpp"

class Button final : public UIObject {
public:
    Button(QRectF bounds, QString text, float fontSize, QColor fillColor, std::function<void()> onClick);

    void handleMouse(const QPointF& mousePos, bool leftPressedEdge);

    void update(float deltaTimeSeconds) override;
    void draw(QPainter* painter) const override;

private:
    QString m_text;
    float m_fontSize = 14.0f;
    QColor m_fill;
    std::function<void()> m_onClick;

    bool m_hover = false;
    bool m_leftPressedEdge = false;

    QElapsedTimer m_pressTimer;
    int m_debounceMs = 100;
};

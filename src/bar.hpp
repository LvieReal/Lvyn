#pragma once

#include <optional>

#include <QColor>
#include <QPainter>
#include <QPointF>
#include <QRectF>

class Bar {
public:
    Bar() = default;

    Bar(float width, float height, float strokeWidth, QColor fill, QColor stroke, float lerpFactor, float initialPercentage);

    void setWidth(float width);

    void setHidden(bool hidden);      // starts fade with delay
    void forceHidden(bool hidden);    // immediate

    bool isHidden() const;

    void update(float deltaTimeSeconds, float percentage);
    void draw(QPainter* painter, const QPointF& topLeft) const;
    void draw(QPainter* painter, const QRectF& rect) const;

private:
    float m_width = 0.0f;
    float m_height = 0.0f;
    float m_strokeWidth = 0.0f;
    QColor m_fill;
    QColor m_stroke;

    float m_target = 1.0f;
    float m_value = 1.0f;
    float m_lerpFactor = 0.15f;

    bool m_hiding = false;
    float m_opacity = 1.0f;

    float m_showDelaySeconds = 0.125f;
    float m_delayRemainingSeconds = 0.0f;

    float m_fadeSpeedPerSecond = 10.0f;
};

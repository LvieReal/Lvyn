#pragma once

#include <QColor>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QString>

// Simple stroked text label (stroke defaults to black).
class Label {
public:
    QString text;
    QColor fill = Qt::white;
    QColor stroke = QColor(0, 0, 0);
    float strokeWidth = 3.0f;
    int pixelSize = 22;
    bool bold = true;
    QString family = "Ubuntu";

    void drawInRect(QPainter* painter, const QRectF& rect, Qt::Alignment align = Qt::AlignCenter, float rotationRadians = 0.0f) const;
    void drawAt(QPainter* painter, const QPointF& pos, Qt::Alignment align = Qt::AlignCenter, float rotationRadians = 0.0f) const;
};

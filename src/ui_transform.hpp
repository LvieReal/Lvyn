#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>

// Lightweight UI transform: top-left position + size + optional rotation.
// Rotation is around the rect center by convention.
struct UiTransform {
    QPointF position; // top-left
    QSizeF size;
    float rotationRadians = 0.0f;

    QRectF rect() const {
        return QRectF(position, size);
    }

    QPointF center() const {
        return QPointF(position.x() + size.width() * 0.5, position.y() + size.height() * 0.5);
    }
};


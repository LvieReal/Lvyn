#pragma once

#include <QPainter>

#include "ui_transform.hpp"

class UIObject {
public:
    explicit UIObject(UiTransform transform) : m_transform(std::move(transform)) {}
    virtual ~UIObject() = default;

    UiTransform transform() const {
        return m_transform;
    }

    QRectF boundsRect() const {
        return m_transform.rect();
    }

    void setBounds(const QRectF& b) {
        m_transform.position = b.topLeft();
        m_transform.size = b.size();
    }

    void setTransform(const UiTransform& t) {
        m_transform = t;
    }

    virtual void update(float deltaTimeSeconds) = 0;
    virtual void draw(QPainter* painter) const = 0;

private:
    UiTransform m_transform;
};

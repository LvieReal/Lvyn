#include "button.hpp"

#include <algorithm>

#include <QFont>

#include "label.hpp"

Button::Button(QRectF bounds, QString text, float fontSize, QColor fillColor, std::function<void()> onClick)
    : UIObject(UiTransform{ bounds.topLeft(), bounds.size(), 0.0f }),
      m_text(std::move(text)),
      m_fontSize(fontSize),
      m_fill(std::move(fillColor)),
      m_onClick(std::move(onClick)) {
    m_pressTimer.start();
}

void Button::handleMouse(const QPointF& mousePos, bool leftPressed) {
    m_hover = boundsRect().contains(mousePos);
    m_leftPressedEdge = leftPressed;
}

void Button::update(float deltaTimeSeconds) {
    (void)deltaTimeSeconds;
    if (!m_onClick)
        return;

    if (m_hover && m_leftPressedEdge && m_pressTimer.elapsed() > m_debounceMs) {
        m_pressTimer.restart();
        m_onClick();
    }
}

void Button::draw(QPainter* painter) const {
    if (!painter)
        return;

    const QRectF bb = boundsRect();

    painter->save();

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(63, 63, 63));
    painter->drawRoundedRect(bb.adjusted(-4, -4, 4, 4), 6, 6);

    QColor fill = m_fill;
    if (m_hover)
        fill = fill.lighter(110);

    painter->setBrush(fill);
    painter->drawRoundedRect(bb, 6, 6);

    // bottom shade
    painter->setBrush(QColor(0, 0, 0, 80));
    painter->drawRoundedRect(QRectF(bb.x(), bb.y() + bb.height() * 0.6f, bb.width(), bb.height() * 0.4f), 6, 6);

    Label lbl;
    lbl.text = m_text;
    lbl.bold = true;
    lbl.pixelSize = static_cast<int>(m_fontSize);
    lbl.fill = Qt::white;
    lbl.drawInRect(painter, bb, Qt::AlignCenter);

    painter->restore();
}

#include "bar.hpp"

#include <algorithm>

namespace {

float clamp01(float v) {
    return std::clamp(v, 0.0f, 1.0f);
}

} // namespace

Bar::Bar(float width, float height, float strokeWidth, QColor fill, QColor stroke, float lerpFactor, float initialPercentage)
    : m_width(width),
      m_height(height),
      m_strokeWidth(strokeWidth),
      m_fill(std::move(fill)),
      m_stroke(std::move(stroke)),
      m_target(clamp01(initialPercentage)),
      m_value(clamp01(initialPercentage)),
      m_lerpFactor(std::max(0.0f, lerpFactor)) {}

void Bar::setWidth(float width) {
    m_width = std::max(0.0f, width);
}

void Bar::setHidden(bool hidden) {
    if (m_hiding && !hidden)
        m_delayRemainingSeconds = m_showDelaySeconds;
    m_hiding = hidden;
}

void Bar::forceHidden(bool hidden) {
    m_hiding = hidden;
    m_delayRemainingSeconds = 0.0f;
    m_opacity = hidden ? 0.0f : 1.0f;
}

bool Bar::isHidden() const {
    return m_hiding && m_opacity <= 0.0f;
}

void Bar::update(float deltaTimeSeconds, float percentage) {
    const float dt = std::max(0.0f, deltaTimeSeconds);
    m_target = clamp01(percentage);

    // Lerp value.
    m_value += (m_target - m_value) * m_lerpFactor;

    // Delay.
    m_delayRemainingSeconds = std::max(0.0f, m_delayRemainingSeconds - dt);

    // Fade.
    if (m_hiding) {
        m_opacity = std::max(0.0f, m_opacity - m_fadeSpeedPerSecond * dt);
    } else {
        if (m_delayRemainingSeconds <= 0.0f)
            m_opacity = std::min(1.0f, m_opacity + m_fadeSpeedPerSecond * dt);
    }
}

void Bar::draw(QPainter* painter, const QPointF& topLeft) const {
    draw(painter, QRectF(topLeft.x(), topLeft.y(), m_width, m_height));
}

void Bar::draw(QPainter* painter, const QRectF& rect) const {
    if (!painter || isHidden() || rect.width() <= 0.0f || rect.height() <= 0.0f)
        return;

    painter->save();
    painter->setOpacity(painter->opacity() * m_opacity);

    const QRectF outer = rect;
    const float radius = std::max(1.0f, static_cast<float>(outer.height()) * 0.5f);

    painter->setBrush(m_stroke);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(outer, radius, radius);

    const float innerWidth = std::max(static_cast<float>(outer.height()) - 2.0f * m_strokeWidth,
        static_cast<float>(outer.width()) * m_value - 2.0f * m_strokeWidth);
    const QRectF inner(outer.x() + m_strokeWidth, outer.y() + m_strokeWidth, innerWidth, outer.height() - 2.0f * m_strokeWidth);

    painter->setBrush(m_fill);
    painter->drawRoundedRect(inner, radius, radius);

    painter->restore();
}

#include "button_group.hpp"

#include <algorithm>

void ButtonGroup::setButtonsCentered(
    const QPointF& center,
    const std::vector<QString>& labels,
    float fontSize,
    const std::vector<QColor>& fillColors,
    const std::vector<std::function<void()>>& onClick,
    float gapWidth) {
    m_buttons.clear();
    if (labels.empty())
        return;

    const size_t n = labels.size();
    m_buttons.reserve(n);

    // Approximate bounds based on font size (simple; can be refined later).
    auto measure = [fontSize](const QString& s) -> QSizeF {
        const float w = std::max(80.0f, fontSize * 0.6f * static_cast<float>(s.size()) + 30.0f);
        const float h = std::max(30.0f, fontSize * 1.4f);
        return QSizeF(w, h);
    };

    std::vector<QSizeF> sizes;
    sizes.reserve(n);
    float widthSum = gapWidth * static_cast<float>(std::max<size_t>(0, n - 1));
    for (size_t i = 0; i < n; i++) {
        const QSizeF sz = measure(labels[i]);
        sizes.push_back(sz);
        widthSum += static_cast<float>(sz.width());
    }

    float x = static_cast<float>(center.x()) - widthSum * 0.5f;
    for (size_t i = 0; i < n; i++) {
        const QSizeF sz = sizes[i];
        const QColor fill = (i < fillColors.size()) ? fillColors[i] : QColor(163, 255, 251);
        const auto cb = (i < onClick.size()) ? onClick[i] : std::function<void()>();

        const QRectF bounds(x, center.y() - sz.height() * 0.5, sz.width(), sz.height());
        m_buttons.emplace_back(bounds, labels[i], fontSize, fill, cb);

        x += static_cast<float>(sz.width()) + gapWidth;
    }
}

void ButtonGroup::handleInput(const QPointF& mousePos, bool leftPressedEdge) {
    for (auto& b : m_buttons)
        b.handleMouse(mousePos, leftPressedEdge);
}

void ButtonGroup::update(float deltaTimeSeconds) {
    for (auto& b : m_buttons)
        b.update(deltaTimeSeconds);
}

void ButtonGroup::draw(QPainter* painter) const {
    for (const auto& b : m_buttons)
        b.draw(painter);
}


#pragma once

#include <functional>
#include <vector>

#include <QColor>
#include <QPointF>
#include <QString>

#include "button.hpp"

class ButtonGroup {
public:
    ButtonGroup() = default;

    void setButtonsCentered(
        const QPointF& center,
        const std::vector<QString>& labels,
        float fontSize,
        const std::vector<QColor>& fillColors,
        const std::vector<std::function<void()>>& onClick,
        float gapWidth = 20.0f);

    void handleInput(const QPointF& mousePos, bool leftPressedEdge);
    void update(float deltaTimeSeconds);
    void draw(QPainter* painter) const;

private:
    std::vector<Button> m_buttons;
};


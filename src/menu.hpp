#pragma once

#include <optional>

#include <QColor>
#include <QElapsedTimer>
#include <QPointF>
#include <QSizeF>
#include <QString>

#include "button.hpp"
#include "ui_transform.hpp"

class InputState;

class Menu {
public:
    Menu();

    void reset(const QSizeF& viewportSize);

    void update(float deltaTimeSeconds, const QSizeF& viewportSize, const InputState& input);
    void draw(QPainter* painter, const QSizeF& viewportSize) const;

    std::optional<QString> consumeSpawnRequest();

private:
    void layout(const QSizeF& viewportSize);
    void applyTextInput(const InputState& input);

private:
    QString m_username;
    UiTransform m_nameBox;

    Button m_spawnButton;

    bool m_nameFocused = false;
    std::optional<QString> m_spawnRequest;

    QElapsedTimer m_cursorClock;
};

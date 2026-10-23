#pragma once

#include <optional>

#include <QColor>
#include <QElapsedTimer>
#include <QPainter>
#include <QSizeF>
#include <QString>

class InputState;

struct DeathInfo {
    QString killerName;
    QString buildName;
    float score = 0.0f;
    int level = 1;
    float aliveSeconds = 0.0f;

    QColor fillColor;
    QColor strokeColor;
};

class DeathScreen {
public:
    void reset(const DeathInfo& info);

    void update(float deltaTimeSeconds, const InputState& input);
    void draw(QPainter* painter, const QSizeF& viewportSize) const;

    const DeathInfo& info() const {
        return m_info;
    }

    bool consumeExitRequest();

private:
    static QString formatAliveTime(float seconds);

private:
    DeathInfo m_info;
    float m_elapsedSeconds = 0.0f;
    bool m_exitRequested = false;
};

#pragma once

#include <array>

#include <QSizeF>
#include <QString>
#include <QImage>

#include "bar.hpp"
#include "stats.hpp"

class Player;
class InputState;

class InGameHud {
public:
    void reset(const QSizeF& viewportSize);

    void update(float deltaTimeSeconds, const QSizeF& viewportSize, Player* player, const InputState& input);
    void draw(QPainter* painter, const QSizeF& viewportSize, const Player* player) const;

private:
    Bar m_levelBar;
    Bar m_scoreBar;
    std::array<Bar, Stats::COUNT> m_upgradeBars;
    float m_upgradePanelT = 0.0f;
    bool m_inited = false;

    // Cached label images for the upgrade panel to reduce per-frame text path work.
    std::array<QImage, Stats::COUNT> m_upgradeLabelImages;
    mutable QImage m_upgradePointsImage;
    mutable int m_cachedUpgradePoints = -1;
};

#include "ingame_hud.hpp"

#include <algorithm>
#include <numbers>

#include <QPainter>
#include <QLocale>

#include "player.hpp"
#include "score_handler.hpp"
#include "settings.hpp"
#include "label.hpp"
#include "input_state.hpp"
#include "stat_upgrade_catalog.hpp"

namespace {

struct UpgradePanelLayout {
    float panelW = 250.0f;
    float barH = 18.0f;
    float rowGap = 6.0f;
    float padX = 10.0f;
    float padY = 12.0f;
    float marginBottom = 10.0f;

    float panelH = 0.0f;
    float x0 = 0.0f;
    float y0 = 0.0f;

    QRectF triggerRect;

    float rowHeight() const {
        return barH + rowGap;
    }

    QRectF rowRect(size_t ui) const {
        return QRectF(
            x0 + padX,
            y0 + padY + static_cast<float>(ui) * rowHeight(),
            panelW - 2.0f * padX,
            barH);
    }
};

UpgradePanelLayout computeUpgradePanelLayout(const QSizeF& viewportSize, float panelT) {
    UpgradePanelLayout l;
    l.panelH = l.rowHeight() * static_cast<float>(kStatUpgradeUi.size()) + 24.0f;
    l.y0 = viewportSize.height() - l.marginBottom - l.panelH;

    const float hiddenX = -l.panelW;
    l.x0 = hiddenX + (0.0f - hiddenX) * panelT;

    // Trigger uses the bottom-left footprint of the panel (not its current slide position),
    // so hovering the corner can slide it back in.
    l.triggerRect = QRectF(0.0, l.y0, l.panelW, l.panelH);
    return l;
}

QImage makeLabelImage(const QString& text, int pixelSize, const QSize& size) {
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    Label lbl;
    lbl.pixelSize = pixelSize;
    lbl.bold = true;
    lbl.text = text;
    lbl.fill = Qt::white;
    lbl.drawInRect(&p, QRectF(QPointF(0, 0), QSizeF(size)), Qt::AlignVCenter | Qt::AlignLeft);
    return img;
}

void drawPlus(QPainter* painter, const QRectF& r, float stroke, const QColor& col) {
    if (!painter)
        return;

    const float cx = static_cast<float>(r.center().x());
    const float cy = static_cast<float>(r.center().y());
    const float longLen = std::min(static_cast<float>(r.width()), static_cast<float>(r.height())) * 0.60f;
    const float shortLen = std::max(1.0f, stroke);

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(col);
    painter->drawRect(QRectF(cx - shortLen * 0.5f, cy - longLen * 0.5f, shortLen, longLen));
    painter->drawRect(QRectF(cx - longLen * 0.5f, cy - shortLen * 0.5f, longLen, shortLen));
    painter->restore();
}

void drawCircleSector(QPainter* painter, const QPointF& center, float radius, float startDeg, float spanDeg, const QColor& col) {
    if (!painter || radius <= 0.0f)
        return;

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(col);
    const QRectF rr(center.x() - radius, center.y() - radius, radius * 2.0f, radius * 2.0f);
    painter->drawPie(rr, static_cast<int>(std::lround(startDeg * 16.0f)), static_cast<int>(std::lround(spanDeg * 16.0f)));
    painter->restore();
}

void drawUpgradeBarRow(
    QPainter* painter,
    const QRectF& r,
    int maxRects,
    int curRects,
    float strokeWidth,
    const QColor& fillCol,
    const QColor& strokeCol,
    float opacity,
    const QImage& mainLabelImage,
    const QString& smallText,
    bool showPlus,
    bool plusEnabled) {
    if (!painter || r.isEmpty())
        return;

    maxRects = std::max(0, maxRects);
    curRects = std::clamp(curRects, 0, maxRects);

    painter->save();
    opacity = std::clamp(opacity, 0.0f, 1.0f);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(strokeCol.red(), strokeCol.green(), strokeCol.blue(), static_cast<int>(255.0f * opacity)));
    painter->drawRoundedRect(r, r.height() * 0.5, r.height() * 0.5);

    const float height = static_cast<float>(r.height());
    const float radius = std::max(0.0f, height * 0.5f - strokeWidth);
    const float higherOpacity = std::pow(std::clamp(opacity, 0.0f, 1.0f), 0.6f);

    QColor buttonCol = (curRects == maxRects) ? Settings::palette.value("GREY", QColor(153, 153, 153)) : fillCol;

    // Solve rectWidth from: totalWidth = height + ((maxRects+2)*stroke) + (maxRects+1)*rectWidth
    const float totalStroke = (maxRects + 2) * strokeWidth;
    const float rectCount = static_cast<float>(maxRects + 1);
    const float innerAvail = std::max(0.0f, static_cast<float>(r.width()) - height - totalStroke);
    const float rectW = (rectCount > 0.0f) ? (innerAvail / rectCount) : 0.0f;

    // Left filled cap (only if any fill exists).
    if (curRects > 0) {
        const QPointF c(r.left() + height * 0.5f, r.top() + height * 0.5f);
        drawCircleSector(
            painter,
            c,
            radius,
            /*startDeg=*/180.0f,
            /*spanDeg=*/180.0f,
            QColor(fillCol.red(), fillCol.green(), fillCol.blue(), static_cast<int>(255.0f * higherOpacity)));
    }

    float rectX = static_cast<float>(r.left() + height * 0.5f + strokeWidth);
    const float y = static_cast<float>(r.top() + strokeWidth);
    const float h = std::max(0.0f, height - 2.0f * strokeWidth);

    for (int i = 0; i <= maxRects; i++) {
        QColor col = fillCol;
        float xShift = 0.0f;
        float wShift = 0.0f;
        if (i == 0) {
            xShift = -strokeWidth;
            wShift = strokeWidth;
        }
        if (i == maxRects) {
            col = buttonCol;
            wShift = strokeWidth;
        }

        if (i < curRects || i == maxRects) {
            painter->setBrush(QColor(col.red(), col.green(), col.blue(), static_cast<int>(255.0f * higherOpacity)));
            painter->drawRect(QRectF(rectX + xShift, y, rectW + wShift, h));
        }
        rectX += rectW + strokeWidth;
    }

    // Right cap.
    {
        const QPointF c(rectX, r.top() + height * 0.5f);
        drawCircleSector(
            painter,
            c,
            radius,
            /*startDeg=*/0.0f,
            /*spanDeg=*/180.0f,
            QColor(buttonCol.red(), buttonCol.green(), buttonCol.blue(), static_cast<int>(255.0f * higherOpacity)));
    }

    // Cached label image (left).
    if (!mainLabelImage.isNull()) {
        painter->setOpacity(std::min(1.0f, opacity * 1.2f));
        painter->drawImage(QPointF(r.left() + 8.0f, r.top() + (r.height() - mainLabelImage.height()) * 0.5), mainLabelImage);
        painter->setOpacity(1.0);
    }

    // Small text (right, inside bar before the plus endcap).
    if (!smallText.isEmpty()) {
        Label small;
        small.pixelSize = static_cast<int>(height * 0.8f);
        small.bold = true;
        small.text = smallText;
        small.fill = QColor(255, 255, 255, static_cast<int>(255.0f * std::min(1.0f, higherOpacity * 1.5f)));
        small.drawInRect(painter, r.adjusted(0, 0, -height, 0), Qt::AlignVCenter | Qt::AlignRight);
    }

    if (showPlus) {
        const QRectF plusRect(r.right() - height, r.top(), height, height);
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, plusEnabled ? 55 : 25));
        painter->drawEllipse(plusRect.adjusted(2, 2, -2, -2));
        painter->restore();

        drawPlus(
            painter,
            plusRect,
            strokeWidth * 1.5f,
            QColor(strokeCol.red(), strokeCol.green(), strokeCol.blue(), plusEnabled ? 255 : 120));
    }

    painter->restore();
}

QString formatScore(float score) {
    const qint64 s = static_cast<qint64>(std::llround(std::max(0.0f, score)));
    return QLocale::system().toString(s);
}

} // namespace

void InGameHud::reset(const QSizeF& viewportSize) {
    (void)viewportSize;
    m_inited = false;
    m_upgradePanelT = 0.0f;
    m_upgradePointsImage = QImage();
    m_cachedUpgradePoints = -1;
}

void InGameHud::update(float deltaTimeSeconds, const QSizeF& viewportSize, Player* player, const InputState& input) {
    if (!player)
        return;

    if (!m_inited) {
        const float barW = 450.0f;
        const float barH = 25.0f;
        const QColor levelCol = Settings::palette.value("LEVELBAR", QColor(255, 232, 105));
        const QColor scoreCol = Settings::palette.value("SCORE_GREEN", QColor(67, 255, 145));
        const QColor stroke = Settings::palette.value("BAR_GREY", QColor(20, 20, 20));

        m_levelBar = Bar(barW, barH, 3.0f, levelCol, stroke, 0.08f, 0.0f);
        m_scoreBar = Bar(barW * 2.0f / 3.0f, barH * 0.8f, 2.0f, scoreCol, stroke, 0.08f, 0.0f);

        for (size_t ui = 0; ui < kStatUpgradeUi.size(); ui++) {
            const auto& entry = kStatUpgradeUi[ui];
            const QColor c = Settings::palette.value(entry.paletteKey, QColor(255, 255, 255));
            m_upgradeBars[ui] = Bar(220.0f, 18.0f, 2.0f, c, stroke, 0.2f, 0.0f);
        }

        // Cache "[n] Stat Name" row labels for performance (panel draw is expensive otherwise).
        const QSize labelSize(200, 20);
        for (size_t ui = 0; ui < kStatUpgradeUi.size(); ui++) {
            const int hotkey = static_cast<int>(ui) + 1;
            const QString t = QString("[%1] %2").arg(hotkey).arg(kStatUpgradeUi[ui].name);
            m_upgradeLabelImages[ui] = makeLabelImage(t, 14, labelSize);
        }

        m_inited = true;
    }

    const int level = player->level;
    const float score = player->score;
    const float start = ScoreHandler::levelToScore(level);
    const float next = ScoreHandler::levelToScore(level + 1);
    float pct = 1.0f;
    if (level < ScoreHandler::maxPlayerLevel && next > start)
        pct = std::clamp((score - start) / (next - start), 0.0f, 1.0f);

    m_levelBar.update(deltaTimeSeconds, pct);

    // Score bar is just normalized to an arbitrary cap for now (until leaderboard exists).
    const float scorePct = std::clamp(score / std::max(1.0f, next), 0.0f, 1.0f);
    m_scoreBar.update(deltaTimeSeconds, scorePct);

    for (size_t ui = 0; ui < kStatUpgradeUi.size(); ui++) {
        const int statIndex = kStatUpgradeUi[ui].statIndex;
        const int cap = player->getStatCap(statIndex);
        const int val = player->stats.getStat(statIndex);
        const float p = (cap > 0) ? std::clamp(static_cast<float>(val) / static_cast<float>(cap), 0.0f, 1.0f) : 0.0f;
        m_upgradeBars[ui].update(deltaTimeSeconds, p);
    }

    const int remaining = std::max(0, player->maxStatPoints - player->usedStatPoints);
    const UpgradePanelLayout layout = computeUpgradePanelLayout(viewportSize, m_upgradePanelT);
    const bool hover = layout.triggerRect.contains(input.mousePosition);
    const bool targetVisible = (remaining > 0) || hover;

    const float target = targetVisible ? 1.0f : 0.0f;
    const float k = std::clamp(deltaTimeSeconds * 10.0f, 0.0f, 1.0f);
    m_upgradePanelT += (target - m_upgradePanelT) * k;

    if (m_upgradePanelT > 0.01f && input.isMousePressed(Qt::LeftButton) && remaining > 0) {
        const UpgradePanelLayout l = computeUpgradePanelLayout(viewportSize, m_upgradePanelT);
        for (size_t ui = 0; ui < kStatUpgradeUi.size(); ui++) {
            const QRectF barRect = l.rowRect(ui);
            const QRectF plusRect(barRect.right() - l.barH, barRect.top(), l.barH, l.barH);
            if (!plusRect.contains(input.mousePosition))
                continue;

            const int statIndex = kStatUpgradeUi[ui].statIndex;
            player->incrementStat(statIndex);
            break;
        }
    }

    // Keep widths responsive to viewport.
    (void)viewportSize;
}

void InGameHud::draw(QPainter* painter, const QSizeF& viewportSize, const Player* player) const {
    if (!painter || !player || !m_inited)
        return;

    painter->save();

    const float barW = 450.0f;
    const float barH = 25.0f;
    const QPointF levelPos((viewportSize.width() - barW) * 0.5f, viewportSize.height() - barH - 10.0f);

    // Score bar above level bar.
    const float scoreW = barW * 2.0f / 3.0f;
    const float scoreH = barH * 0.8f;
    const QPointF scorePos((viewportSize.width() - scoreW) * 0.5f, levelPos.y() - scoreH - 3.0f);

    m_levelBar.draw(painter, levelPos);
    m_scoreBar.draw(painter, scorePos);

    {
        Label lvl;
        lvl.pixelSize = 18;
        lvl.bold = true;
        lvl.text = QString("Lvl %1").arg(player->level);
        lvl.drawInRect(painter, QRectF(levelPos.x(), levelPos.y(), barW, barH), Qt::AlignCenter);

        Label scoreLbl;
        scoreLbl.pixelSize = 16;
        scoreLbl.bold = true;
        scoreLbl.text = formatScore(player->score);
        scoreLbl.drawInRect(painter, QRectF(scorePos.x(), scorePos.y(), scoreW, scoreH), Qt::AlignCenter);

        Label name;
        name.pixelSize = 40;
        name.bold = true;
        name.text = player->username;
        name.drawInRect(painter, QRectF(0, levelPos.y() - 80.0f, viewportSize.width(), 60.0f), Qt::AlignHCenter | Qt::AlignVCenter);
    }

    // Upgrade bars (bottom-left)
    const UpgradePanelLayout l = computeUpgradePanelLayout(viewportSize, m_upgradePanelT);

    if (m_upgradePanelT > 0.001f) {
        const int remaining = std::max(0, player->maxStatPoints - player->usedStatPoints);

        // Cache the points label image (changes infrequently, expensive stroked text).
        if (remaining != m_cachedUpgradePoints || m_upgradePointsImage.isNull()) {
            m_cachedUpgradePoints = remaining;
            m_upgradePointsImage = makeLabelImage(QString("%1x").arg(remaining), 28, QSize(48, 48));
        }
        painter->save();
        painter->translate(QPointF(l.x0 + l.panelW, l.y0 - 14.0f) + QPointF(14.0f, 14.0f));
        painter->rotate(30.0);
        painter->translate(QPointF(-14.0f, -14.0f));
        painter->drawImage(QPointF(0, 0), m_upgradePointsImage);
        painter->restore();

        for (size_t ui = 0; ui < kStatUpgradeUi.size(); ui++) {
            const int statIndex = kStatUpgradeUi[ui].statIndex;
            const bool enabled = remaining > 0 && player->stats.getStat(statIndex) < player->getStatCap(statIndex);
            const QRectF barRect = l.rowRect(ui);

            const int cap = std::max(0, player->getStatCap(statIndex));
            const int val = std::clamp(player->stats.getStat(statIndex), 0, cap);
            const QColor strokeCol = Settings::palette.value("BAR_GREY", QColor(20, 20, 20));
            const QColor fillCol = Settings::palette.value(kStatUpgradeUi[ui].paletteKey, QColor(255, 255, 255));
            const QString smallText = QString("%1/%2").arg(val).arg(std::max(1, cap));

            drawUpgradeBarRow(
                painter,
                barRect,
                cap,
                val,
                2.0f,
                fillCol,
                strokeCol,
                m_upgradePanelT,
                m_upgradeLabelImages[ui],
                smallText,
                /*showPlus=*/true,
                /*plusEnabled=*/enabled);
        }
    }

    painter->restore();
}

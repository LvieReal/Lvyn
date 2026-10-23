#include "death_screen.hpp"

#include <algorithm>
#include <cmath>

#include <QPainter>
#include <QSizeF>
#include <QLocale>

#include "input_state.hpp"
#include "label.hpp"

namespace {

QString formatScoreInt(float score) {
    const qint64 s = static_cast<qint64>(std::llround(std::max(0.0f, score)));
    return QLocale::system().toString(s);
}

} // namespace

void DeathScreen::reset(const DeathInfo& info) {
    m_info = info;
    m_elapsedSeconds = 0.0f;
    m_exitRequested = false;
}

void DeathScreen::update(float deltaTimeSeconds, const InputState& input) {
    m_elapsedSeconds += std::max(0.0f, deltaTimeSeconds);

    // Any click / enter returns to menu for now (parity with Java having "continue/quit" elsewhere).
    if (input.isMousePressed(Qt::LeftButton) || input.isKeyPressed(Qt::Key_Return) || input.isKeyPressed(Qt::Key_Enter)) {
        m_exitRequested = true;
    }
}

bool DeathScreen::consumeExitRequest() {
    const bool r = m_exitRequested;
    m_exitRequested = false;
    return r;
}

QString DeathScreen::formatAliveTime(float seconds) {
    seconds = std::max(0.0f, seconds);
    const int s = static_cast<int>(std::floor(seconds));
    const int mm = s / 60;
    const int ss = s % 60;
    if (mm <= 0)
        return QString("%1s").arg(ss);
    return QString("%1m %2s").arg(mm).arg(ss, 2, 10, QChar('0'));
}

void DeathScreen::draw(QPainter* painter, const QSizeF& viewportSize) const {
    if (!painter)
        return;

    painter->save();

    // Fade-in overlay (matches Java: alpha ramps up with frames, capped).
    const float a = std::clamp(m_elapsedSeconds * 2.0f, 0.0f, 1.0f);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, static_cast<int>(100.0f * a)));
    painter->drawRect(QRectF(QPointF(0, 0), viewportSize));

    float totHeight = 400.0f;
    float yPos = (viewportSize.height() - totHeight) * 0.5f;

    // "You were killed by:"
    {
        Label t;
        t.pixelSize = 28;
        t.bold = true;
        t.text = "You were killed by:";
        t.drawInRect(painter, QRectF(0, yPos, viewportSize.width(), 40.0f), Qt::AlignHCenter | Qt::AlignVCenter);
    }
    yPos += 40.0f;

    // Killer name
    {
        Label k;
        k.pixelSize = 50;
        k.bold = true;
        k.text = m_info.killerName.isEmpty() ? "Unknown" : m_info.killerName;
        k.drawInRect(painter, QRectF(0, yPos, viewportSize.width(), 56.0f), Qt::AlignHCenter | Qt::AlignVCenter);
    }
    yPos += 50.0f;

    // Text block + tank icon
    const float gap = 38.0f;
    const float blockW = 250.0f;
    const float padding = (blockW - gap * 3.0f) * 0.5f;
    float yPos2 = yPos + padding + 10.0f;
    const float xPos = viewportSize.width() * 0.5f - blockW;

    {
        Label l;
        l.pixelSize = 28;
        l.bold = true;
        l.text = QString("Score:  %1").arg(formatScoreInt(m_info.score));
        l.drawInRect(painter, QRectF(xPos, yPos2 - 20.0f, blockW, 40.0f), Qt::AlignLeft | Qt::AlignVCenter);
        yPos2 += gap;

        l.text = QString("Level:  %1").arg(m_info.level);
        l.drawInRect(painter, QRectF(xPos, yPos2 - 20.0f, blockW, 40.0f), Qt::AlignLeft | Qt::AlignVCenter);
        yPos2 += gap;

        l.text = QString("Time Alive:  %1").arg(formatAliveTime(m_info.aliveSeconds));
        l.drawInRect(painter, QRectF(xPos, yPos2 - 20.0f, blockW + 140.0f, 40.0f), Qt::AlignLeft | Qt::AlignVCenter);
    }

    // Tank icon (simple, since we don't have textures yet).
    const QRectF iconRect(viewportSize.width() * 0.5f + blockW * 0.5f, yPos, blockW, blockW);
    {
        painter->save();
        painter->translate(iconRect.center());
        painter->rotate(-45.0);

        const float r = static_cast<float>(iconRect.width()) * 0.28f;

        // Barrel
        painter->setBrush(QColor(160, 160, 160));
        painter->setPen(QPen(m_info.strokeColor.isValid() ? m_info.strokeColor : QColor(80, 80, 80), 8));
        painter->drawRect(QRectF(0.0, -r * 0.35, r * 1.6, r * 0.7));

        painter->setPen(QPen(m_info.strokeColor.isValid() ? m_info.strokeColor : QColor(80, 80, 80), 8));
        painter->setBrush(m_info.fillColor.isValid() ? m_info.fillColor : QColor(0, 178, 225));
        painter->drawEllipse(QPointF(0, 0), r, r);

        painter->restore();
    }

    yPos += blockW;

    // Build name below icon.
    {
        Label b;
        b.pixelSize = 30;
        b.bold = true;
        b.text = m_info.buildName;
        b.drawInRect(painter, QRectF(iconRect.left(), yPos, iconRect.width(), 40.0f), Qt::AlignHCenter | Qt::AlignVCenter);
    }

    painter->restore();
}

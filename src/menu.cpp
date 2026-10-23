#include "menu.hpp"

#include <algorithm>

#include <QPainter>

#include "input_state.hpp"
#include "settings.hpp"
#include "label.hpp"

namespace {

QString sanitizeUsername(QString s) {
    s = s.left(24);
    QString out;
    out.reserve(s.size());
    for (const auto& ch : s) {
        if (ch.isLetterOrNumber() || ch == ' ' || ch == '_' || ch == '-')
            out.append(ch);
    }
    if (out.trimmed().isEmpty())
        out = "unnamed";
    return out;
}

} // namespace

Menu::Menu()
    : m_spawnButton(QRectF(), "Spawn", 26.0f, Settings::palette.value("BLUE", QColor(0, 178, 225)), [this]() {
          m_spawnRequest = sanitizeUsername(m_username);
      }) {}

void Menu::reset(const QSizeF& viewportSize) {
    m_spawnRequest.reset();
    m_nameFocused = false;
    m_username.clear();
    m_cursorClock.restart();
    layout(viewportSize);
}

void Menu::layout(const QSizeF& viewportSize) {
    const float cx = static_cast<float>(viewportSize.width()) * 0.5f;
    const float cy = static_cast<float>(viewportSize.height()) * 0.5f;

    const float boxW = 520.0f;
    const float boxH = 56.0f;
    m_nameBox.position = QPointF(cx - boxW * 0.5f, cy);
    m_nameBox.size = QSizeF(boxW, boxH);
    m_nameBox.rotationRadians = 0.0f;

    const QRectF nameRect = m_nameBox.rect();
    const QRectF btn(cx - 170.0f, nameRect.y() + nameRect.height() + 18.0f, 340.0f, 60.0f);
    m_spawnButton.setBounds(btn);
}

void Menu::applyTextInput(const InputState& input) {
    if (!m_nameFocused)
        return;

    if (input.backspacePressed && !m_username.isEmpty())
        m_username.chop(1);

    if (!input.textInput.isEmpty())
        m_username.append(input.textInput);
}

void Menu::update(float deltaTimeSeconds, const QSizeF& viewportSize, const InputState& input) {
    layout(viewportSize);

    // Focus name box on click.
    if (input.isMousePressed(Qt::LeftButton)) {
        const bool prev = m_nameFocused;
        m_nameFocused = m_nameBox.rect().contains(input.mousePosition);
        if (m_nameFocused && !prev)
            m_cursorClock.restart();
    }

    applyTextInput(input);

    // Spawn on Enter.
    if (input.isKeyPressed(Qt::Key_Return) || input.isKeyPressed(Qt::Key_Enter))
        m_spawnRequest = sanitizeUsername(m_username);

    m_spawnButton.handleMouse(input.mousePosition, input.isMousePressed(Qt::LeftButton));
    m_spawnButton.update(deltaTimeSeconds);
}

std::optional<QString> Menu::consumeSpawnRequest() {
    auto r = m_spawnRequest;
    m_spawnRequest.reset();
    return r;
}

void Menu::draw(QPainter* painter, const QSizeF& viewportSize) const {
    if (!painter)
        return;

    painter->save();

    // Dark overlay
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 170));
    painter->drawRect(QRectF(QPointF(0, 0), viewportSize));

    // Username box
    painter->setBrush(QColor(20, 20, 20, 180));
    painter->setPen(QPen(QColor(63, 63, 63), 3));
    painter->drawRoundedRect(m_nameBox.rect(), 8, 8);

    {
        Label txt;
        txt.pixelSize = 28;
        txt.bold = true;
        txt.fill = Qt::white;
        txt.text = m_username;
        const QRectF r = m_nameBox.rect().adjusted(14, 0, -14, 0);
        if (!m_username.isEmpty()) {
            txt.drawInRect(painter, r, Qt::AlignVCenter | Qt::AlignLeft);
        } else {
            txt.fill = QColor(255, 255, 255, 90);
            txt.text = "The tale unfolds of...";
            txt.drawInRect(painter, r, Qt::AlignVCenter | Qt::AlignLeft);
        }

        // Blinking caret to indicate the box is active.
        if (m_nameFocused) {
            const qint64 ms = m_cursorClock.isValid() ? m_cursorClock.elapsed() : 0;
            const bool visible = ((ms / 500) % 2) == 0;
            if (visible) {
                QFont font("Ubuntu");
                font.setPixelSize(28);
                font.setBold(false);

                const QFontMetricsF fm(font);
                const double w = fm.horizontalAdvance(m_username);
                const double top = r.center().y() - fm.height() * 0.5;
                const double x = r.left() + w + 2.0;

                painter->save();
                painter->setPen(QPen(QColor(255, 255, 255, 200), 2));
                painter->drawLine(QPointF(x, top + 6.0), QPointF(x, top + fm.height() - 6.0));
                painter->restore();
            }
        }
    }

    // Spawn button
    m_spawnButton.draw(painter);

    painter->restore();
}

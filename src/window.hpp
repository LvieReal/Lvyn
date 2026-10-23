#pragma once

#include <algorithm>
#include <cmath>

#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWidget>
#include <QResizeEvent>
#include <QTimer>
#include <QUrl>

#include "arena.hpp"
#include "arena_grid.hpp"
#include "arena_view_item.hpp"
#include "ui_view_item.hpp"
#include "game_manager.hpp"
#include "input_state.hpp"
#include "settings.hpp"

class Window : public QMainWindow {
public:
    Window(GameManager& gameManager, Arena* arena)
        : m_gameManager(gameManager), m_arena(arena) {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);

        resize(400, 400);

        m_inputState = gameManager.getInputState();
        m_gameManager.setViewSize(size().toSizeF());

        m_quick = new QQuickWidget(this);
        // Keep keyboard focus on the QMainWindow so our keyPressEvent sees text input.
        m_quick->setFocusPolicy(Qt::NoFocus);
        m_quick->setResizeMode(QQuickWidget::SizeRootObjectToView);
        setCentralWidget(m_quick);

        m_root = new QQuickItem();
        m_root->setWidth(width());
        m_root->setHeight(height());

        // gpu accelerated arena grid renderer
        m_grid = new GridItem(&m_gameManager);
        m_grid->setParent(m_root);
        m_grid->setParentItem(m_root);
        m_grid->setZ(0.0);

        m_arenaView = new ArenaViewItem(m_gameManager, m_arena);
        m_arenaView->setParent(m_root);
        m_arenaView->setParentItem(m_root);
        m_arenaView->setZ(1.0);

        m_uiView = new UiViewItem(m_gameManager);
        m_uiView->setParent(m_root);
        m_uiView->setParentItem(m_root);
        m_uiView->setZ(2.0);

        auto syncItemSizes = [this]() {
            const qreal w = m_root->width();
            const qreal h = m_root->height();
            m_grid->setWidth(w);
            m_grid->setHeight(h);
            m_arenaView->setWidth(w);
            m_arenaView->setHeight(h);
            m_uiView->setWidth(w);
            m_uiView->setHeight(h);
        };

        connect(m_root, &QQuickItem::widthChanged, this, syncItemSizes);
        connect(m_root, &QQuickItem::heightChanged, this, syncItemSizes);
        syncItemSizes();

        m_quick->setContent(QUrl(), nullptr, m_root);

        m_renderTimer = new QTimer(this);
        m_renderTimer->setTimerType(Qt::PreciseTimer);
        m_renderTimer->setSingleShot(true);
        m_renderTimer->callOnTimeout([this]() { onRenderFrame(); });

        m_renderClock.start();
        m_nextFrameMs = 0;
        m_renderTimer->start(0);
    }

    void resizeEvent(QResizeEvent* event) override {
        m_gameManager.setViewSize(event->size().toSizeF());
        event->accept();
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (!event->isAutoRepeat()) {
            if (std::find(m_inputState->keysDown.begin(), m_inputState->keysDown.end(), event->key()) == m_inputState->keysDown.end())
                m_inputState->keysDown.push_back(event->key());
            if (std::find(m_inputState->keysPressed.begin(), m_inputState->keysPressed.end(), event->key()) == m_inputState->keysPressed.end())
                m_inputState->keysPressed.push_back(event->key());
        }

        if (event->key() == Qt::Key_Backspace) {
            m_inputState->backspacePressed = true;
        } else {
            const QString t = event->text();
            // Filter out control characters; keep printable text (for menu username entry).
            for (const auto& ch : t) {
                if (!ch.isNull() && ch.isPrint())
                    m_inputState->textInput.append(ch);
            }
        }

        event->accept();
    }

    void keyReleaseEvent(QKeyEvent* event) override {
        if (event->isAutoRepeat()) {
            event->accept();
            return;
        }

        auto it = std::find(m_inputState->keysDown.begin(), m_inputState->keysDown.end(), event->key());
        if (it != m_inputState->keysDown.end())
            m_inputState->keysDown.erase(it);

        event->accept();
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_inputState)
            m_inputState->mousePosition = event->position();
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override {
        setFocus();
        if (m_inputState) {
            m_inputState->mousePosition = event->position();
            if (std::find(m_inputState->mouseDown.begin(), m_inputState->mouseDown.end(), event->button()) == m_inputState->mouseDown.end())
                m_inputState->mouseDown.push_back(event->button());
            if (std::find(m_inputState->mousePressed.begin(), m_inputState->mousePressed.end(), event->button()) == m_inputState->mousePressed.end())
                m_inputState->mousePressed.push_back(event->button());
        }
        // Ensure we always receive the matching release even if the cursor leaves the window.
        grabMouse();
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (m_inputState) {
            m_inputState->mousePosition = event->position();
            auto it = std::find(m_inputState->mouseDown.begin(), m_inputState->mouseDown.end(), event->button());
            if (it != m_inputState->mouseDown.end())
                m_inputState->mouseDown.erase(it);
        }
        if (m_inputState && m_inputState->mouseDown.empty())
            releaseMouse();
        event->accept();
    }

    void focusOutEvent(QFocusEvent* event) override {
        if (m_inputState) {
            m_inputState->keysDown.clear();
            m_inputState->mouseDown.clear();
        }
        releaseMouse();
        QMainWindow::focusOutEvent(event);
    }

private:
    void onRenderFrame() {
        // View/camera state is read at draw time from GameManager to avoid desync between
        // camera position and object positions (which can advance between setView() and paint()).
        m_grid->update();
        m_arenaView->update();
        m_uiView->update();

        scheduleNextFrame();
    }

    void scheduleNextFrame() {
        const qint64 periodMs = std::max<qint64>(1, static_cast<qint64>(std::llround(Settings::renderRateMS)));
        const qint64 nowMs = m_renderClock.elapsed();

        if (nowMs > m_nextFrameMs + periodMs * 10) {
            m_nextFrameMs = nowMs;
        }

        m_nextFrameMs += periodMs;
        const qint64 delay = std::max<qint64>(0, m_nextFrameMs - nowMs);
        m_renderTimer->start(static_cast<int>(delay));
    }

private:
    GameManager& m_gameManager;
    InputState* m_inputState = nullptr;
    Arena* m_arena = nullptr;

    QQuickWidget* m_quick = nullptr;
    QQuickItem* m_root = nullptr;
    GridItem* m_grid = nullptr;
    ArenaViewItem* m_arenaView = nullptr;
    UiViewItem* m_uiView = nullptr;

    QTimer* m_renderTimer = nullptr;
    QElapsedTimer m_renderClock;
    qint64 m_nextFrameMs = 0;
};

#pragma once

#include <vector>
#include <algorithm>

#include <Qt>
#include <QPointF>
#include <QString>

class InputState {
public:
    std::vector<int> keysDown;
    std::vector<int> keysPressed; // edge-triggered since last consume()
    QString textInput;            // text typed since last consume()
    bool backspacePressed = false;

    QPointF mousePosition;
    std::vector<Qt::MouseButton> mouseDown;
    std::vector<Qt::MouseButton> mousePressed; // edge-triggered since last consume()

    bool isKeyDown(int key) const {
        return std::find(keysDown.begin(), keysDown.end(), key) != keysDown.end();
    }

    bool isMouseDown(Qt::MouseButton button) const {
        return std::find(mouseDown.begin(), mouseDown.end(), button) != mouseDown.end();
    }

    bool isMousePressed(Qt::MouseButton button) const {
        return std::find(mousePressed.begin(), mousePressed.end(), button) != mousePressed.end();
    }

    bool isKeyPressed(int key) const {
        return std::find(keysPressed.begin(), keysPressed.end(), key) != keysPressed.end();
    }

    void consumeEdges() {
        keysPressed.clear();
        textInput.clear();
        backspacePressed = false;
        mousePressed.clear();
    }
};

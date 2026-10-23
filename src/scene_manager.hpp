#pragma once

#include <optional>

#include "scene.hpp"

class SceneManager {
public:
    Scene scene() const {
        return m_scene;
    }

    void setScene(Scene scene) {
        m_pending = scene;
    }

    void refresh() {
        if (m_pending) {
            m_scene = *m_pending;
            m_pending.reset();
        }
    }

private:
    Scene m_scene = Scene::MENU;
    std::optional<Scene> m_pending;
};


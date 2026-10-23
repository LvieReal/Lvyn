#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>
#include <algorithm>
#include <array>

#include <Qt>
#include <QSizeF>
#include <QPointF>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QPainter>

#include "settings.hpp"
#include "game_object.hpp"
#include "player.hpp"
#include "input_state.hpp"
#include "player_controller.hpp"
#include "polygon.hpp"
#include "polygon_definitions.hpp"
#include "polygon_spawner.hpp"
#include "camera_manager.hpp"
#include "collision_manager.hpp"
#include "tank_definitions.hpp"
#include "bullet.hpp"
#include "scene_manager.hpp"
#include "menu.hpp"
#include "ingame_hud.hpp"
#include "score_handler.hpp"
#include "stat_upgrade_catalog.hpp"
#include "death_screen.hpp"

#include <QRandomGenerator>

class GameManager {
public:
    GameManager() {
        m_inputState = InputState();
        m_localPlayerController = PlayerController();
        m_localPlayerController.setInputState(&m_inputState);

        m_polygonSpawner.setGameManager(this);

        m_camera.setLerpFactor(0.05f);
        m_camera.setZoomLerpFactor(0.05f);
        m_camera.forceSetTarget(QPointF(Settings::arenaWidth * 0.5, Settings::arenaHeight * 0.5));
        m_camera.forceSetZoom(m_viewZoom);

        {
            QString err;
            m_tankDefs = loadTankDefinitionsFromFile("config/TankDefinitions.json", &err);
            if (!m_tankDefs)
                qWarning() << "Failed to load tank definitions:" << err;
        }

        {
            QString err;
            m_polygonDefs = loadPolygonDefinitionsFromFile("config/PolygonDefinitions.json", &err);
            if (!m_polygonDefs) {
                qWarning() << "Failed to load polygon definitions:" << err;
            } else {
                QString spErr;
                if (!m_polygonSpawner.setDefinitions(&(*m_polygonDefs), &spErr)) {
                    qWarning() << "Polygon spawner disabled:" << spErr;
                    m_polygonDefs.reset();
                } else {
                    m_polygonSpawner.reset();
                }
            }
        }

        m_timer.setTimerType(Qt::PreciseTimer);
        m_timer.setInterval(static_cast<int>(Settings::physicsRateMS));
        m_timer.callOnTimeout([this]() { onTick(); });

        m_timer.start();
        m_elapsedTimer.start();
    }

    InputState* getInputState() {
        return &m_inputState;
    }

    Player* getLocalPlayer() {
        return m_localPlayer;
    }

    QPointF getViewPos() const {
        return m_camera.target();
    }
    void setViewSize(const QSizeF& newSize) {
        m_viewSize = newSize;
    }
    QSizeF getViewSize() const {
        return m_viewSize;
    }
    float getViewZoom() const {
        return m_viewZoom;
    }
    void setViewZoom(const float& newZoom) {
        m_viewZoom = newZoom;
    }

    const std::unordered_map<int, GameObject*>& getObjects() const {
        return m_gameObjects;
    }
    void addObject(GameObject* object) {
        m_gameObjects[object->getId()] = object;
    }
    void removeObject(GameObject* object) {
        m_gameObjects.erase(object->getId());
    }

    void onTick() {
        m_deltaTime = std::max(std::min(m_elapsedTimer.restart() / 1000.0f, 1.0f), 0.0f);
        Settings::physicsRateCounter += m_deltaTime;

        while (Settings::physicsRateCounter >= Settings::physicsRate) {
            m_scene.refresh();
            updateUi(Settings::physicsRate);

            if (m_scene.scene() == Scene::GAME && m_localPlayer) {
                m_localPlayerController.updateMovement();
                m_localPlayerController.updateAimAndFire(getViewPos(), m_viewSize, m_viewZoom);
            }

            if (m_polygonDefs)
                m_polygonSpawner.update();

            std::vector<GameObject*> objects;
            objects.reserve(m_gameObjects.size());
            for (const auto& [id, obj] : m_gameObjects) {
                (void)id;
                if (obj)
                    objects.push_back(obj);
            }
            for (auto* obj : objects)
                obj->update(Settings::physicsRate);

            m_collisionManager.updateCollision(objects);

            if (m_scene.scene() == Scene::GAME && m_localPlayer) {
                m_camera.setTargetZoom(m_localPlayer->getZoom());
            } else if (m_scene.scene() == Scene::DEATH && m_deathCameraHost) {
                if (auto* t = dynamic_cast<Tank*>(m_deathCameraHost)) {
                    m_camera.setTargetZoom(t->getZoom());
                }
            }

            m_camera.update(Settings::physicsRate, Settings::physicsRate);
            m_viewZoom = m_camera.zoom();

            collectGarbage();

            m_inputState.consumeEdges();

            Settings::physicsRateCounter -= Settings::physicsRate;
        }
    }

    Scene scene() const {
        return m_scene.scene();
    }

    void setScene(Scene scene) {
        m_scene.setScene(scene);
    }

    void drawUi(QPainter* painter, const QSizeF& viewportSize) const {
        if (m_scene.scene() == Scene::MENU) {
            m_menu.draw(painter, viewportSize);
            return;
        }
        if (m_scene.scene() == Scene::GAME) {
            m_hud.draw(painter, viewportSize, m_localPlayer);
            return;
        }
        if (m_scene.scene() == Scene::DEATH) {
            m_deathScreen.draw(painter, viewportSize);
            return;
        }
    }

    Polygon* spawnPolygon(const QString& polygonId, const QPointF& pos, bool isNestPolygon) {
        if (!m_polygonDefs)
            return nullptr;
        const PolygonDefinition* def = m_polygonDefs->find(polygonId);
        if (!def)
            return nullptr;

        auto ptr = std::make_unique<Polygon>(*def, pos, isNestPolygon);
        Polygon* raw = ptr.get();
        m_ownedObjects.push_back(std::move(ptr));
        addObject(raw);
        return raw;
    }

    Bullet* spawnBullet(const BulletSpawnRequest& req, const Tank& owner) {
        auto ptr = std::make_unique<Bullet>(req.bullet, owner, req.position, req.directionRadians, req.barrelWidth);
        Bullet* raw = ptr.get();
        m_ownedObjects.push_back(std::move(ptr));
        addObject(raw);
        return raw;
    }

private:
    GameObject* pickDeathCameraHost(int preferredGroup) {
        if (preferredGroup >= 0) {
            for (const auto& [id, obj] : m_gameObjects) {
                (void)id;
                if (!obj || obj->pendingDelete || obj->isDead())
                    continue;
                if (obj->group == preferredGroup)
                    return obj;
            }
        }

        for (const auto& [id, obj] : m_gameObjects) {
            (void)id;
            if (!obj || obj->pendingDelete || obj->isDead())
                continue;
            return obj;
        }

        return nullptr;
    }

    void updateUi(float deltaTime) {
        if (m_scene.scene() == Scene::MENU) {
            m_menu.update(deltaTime, m_viewSize, m_inputState);
            if (auto req = m_menu.consumeSpawnRequest()) {
                spawnLocalPlayer(*req);
                m_scene.setScene(Scene::GAME);
                m_hud.reset(m_viewSize);
                // Spawning via a mouse click on the menu button should not start firing immediately.
                m_inputState.mouseDown.clear();
                m_inputState.mousePressed.clear();
                m_deathActive = false;
            }
            return;
        }

        if (m_scene.scene() == Scene::GAME && m_localPlayer) {
            if (m_localPlayer->deathAnimating && !m_deathActive) {
                m_deathActive = true;

                DeathInfo info;
                info.killerName = "Unknown";
                info.buildName = m_localPlayer->buildName();
                info.score = m_localPlayer->score;
                info.level = m_localPlayer->level;
                info.aliveSeconds = m_localPlayer->aliveSeconds;
                info.fillColor = m_localPlayer->fillColor;
                info.strokeColor = m_localPlayer->strokeColor;

                m_deathPreferredGroup = m_localPlayer->killerGroup;
                if (m_deathPreferredGroup >= 0)
                    info.killerName = QString("Group %1").arg(m_deathPreferredGroup);

                m_deathScreen.reset(info);
                m_scene.setScene(Scene::DEATH);
                m_deathTargetWatchSeconds = 9999.0f;
                m_deathCameraHost = nullptr;
                return;
            }

            static constexpr std::array<int, Stats::COUNT> keys = {
                Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8
            };
            for (size_t ui = 0; ui < kStatUpgradeUi.size(); ui++) {
                if (m_inputState.isKeyPressed(keys[ui]))
                    m_localPlayer->incrementStat(kStatUpgradeUi[ui].statIndex);
            }

            m_hud.update(deltaTime, m_viewSize, m_localPlayer, m_inputState);
        } else if (m_scene.scene() == Scene::GAME && !m_localPlayer) {
            // If the local player has been deleted, return to menu for now (death screen later).
            m_scene.setScene(Scene::MENU);
            m_menu.reset(m_viewSize);
            m_deathActive = false;
        } else if (m_scene.scene() == Scene::DEATH) {
            m_deathScreen.update(deltaTime, m_inputState);

            m_deathTargetWatchSeconds += std::max(0.0f, deltaTime);
            const bool hostInvalid = (!m_deathCameraHost || m_deathCameraHost->pendingDelete || m_deathCameraHost->isDead());
            if (hostInvalid || m_deathTargetWatchSeconds >= 5.0f) {
                m_deathCameraHost = pickDeathCameraHost(m_deathPreferredGroup);
                m_deathTargetWatchSeconds = 0.0f;
                if (m_deathCameraHost)
                    m_camera.setHost(m_deathCameraHost);
            }

            if (m_deathScreen.consumeExitRequest()) {
                m_scene.setScene(Scene::MENU);
                m_menu.reset(m_viewSize);
                m_camera.setHost(nullptr);
                m_camera.forceSetTarget(QPointF(Settings::arenaWidth * 0.5, Settings::arenaHeight * 0.5));
                m_deathActive = false;
            }
        }
    }

    void spawnLocalPlayer(const QString& username) {
        if (!m_tankDefs)
            return;

        const TankDefinition* def = m_tankDefs->findByName("tank");
        if (!def)
            return;

        auto p = std::make_unique<Player>();
        p->username = username;
        p->setGameManager(this);
        p->applyTankDefinition(*def);
        p->position = QPointF(QRandomGenerator::global()->bounded(Settings::arenaWidth), QRandomGenerator::global()->bounded(Settings::arenaHeight));

        Player* raw = p.get();
        m_ownedObjects.push_back(std::move(p));
        addObject(raw);

        m_localPlayer = raw;
        m_localPlayerController.setHost(raw);
        m_camera.setHost(raw);
    }

    void collectGarbage() {
        // Remove dead owned objects (bullets, future projectiles, etc).
        std::erase_if(m_ownedObjects, [this](const std::unique_ptr<GameObject>& obj) {
            if (!obj)
                return true;
            if (!obj->pendingDelete)
                return false;
            if (obj.get() == m_localPlayer)
                m_localPlayer = nullptr;
            m_gameObjects.erase(obj->getId());
            return true;
        });
    }

    float m_deltaTime = 0.0f;

    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;

    QSizeF m_viewSize;
    float m_viewZoom = 0.6f;

    Player* m_localPlayer = nullptr;
    PlayerController m_localPlayerController;
    InputState m_inputState;

    std::unordered_map<int, GameObject*> m_gameObjects;
    std::vector<std::unique_ptr<GameObject>> m_ownedObjects;

    std::optional<PolygonDefinitions> m_polygonDefs;
    PolygonSpawner m_polygonSpawner;

    CameraManager m_camera;
    CollisionManager m_collisionManager;

    std::optional<TankDefinitions> m_tankDefs;

    SceneManager m_scene;
    mutable Menu m_menu;
    mutable InGameHud m_hud;

    DeathScreen m_deathScreen;
    bool m_deathActive = false;
    int m_deathPreferredGroup = -1;
    float m_deathTargetWatchSeconds = 0.0f;
    GameObject* m_deathCameraHost = nullptr; // non-owning
};

#pragma once

#include <cmath>
#include <numbers>

#include <algorithm>

#include <QString>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSizeF>
#include <QPolygonF>
#include <QColor>
#include <QPainter>
#include <QPen>

#include "settings.hpp"
#include "bar.hpp"

class GameObject {
public:
    GameObject() {
        m_id = ++totalIds;
        group = m_id;
    }

    virtual ~GameObject() = default;

    float zIndex = 0.0f;

    enum class Geometry {
        Circle,
        Rect,
        Polygon
    };

    QString name{ "" };
    QString display{ "" };

    // Objects always collide each other regardless of group (unless noInternalCollision is true).
    // Objects in different groups should collide/knockback; objects in the same group can be filtered.
    int group = 0;
    int killerGroup = -1;

    Geometry geometry = Geometry::Circle;

    float scale = 1.0f;
    float radius = 0.0f;
    QSizeF rectSize{ 0.0f, 0.0f }; // used when geometry == Rect (world units)
    QPolygonF localPolygon; // used when geometry == Polygon (local space)
    float rotationRadians = 0.0f; // used for Rect/Polygon

    float opacity = 1.0f;

    QColor fillColor{ 241, 78, 84 };
    QColor strokeColor{ Settings::globalStrokeColor };

    float maxHealth = 0.0f;
    float health = 0.0f;

    float damage = 0.0f;
    float damageFactor = 1.0f;

    QPointF position{ 0.0f, 0.0f };
    QPointF velocity{ 0.0f, 0.0f };

    float friction = 0.988f;

    float absorptionFactor = 0.0f;
    float pushFactor = 0.0f;

    float lastAcceleration = 0.0f;

    bool keepInArena = false;
    bool noInternalCollision = false;
    bool spawnProtection = false;
    float spawnProtectionElapsedSeconds = 0.0f;

    // Lifecycle: when a GameObject reaches 0 HP, it enters a timed death animation and is then deleted.
    bool pendingDelete = false;
    bool deathAnimating = false;
    float deathAnimElapsedSeconds = 0.0f;
    float deathAnimDurationSeconds = 0.20f;
    float deathAnimStartScale = 1.0f;
    float deathAnimStartOpacity = 1.0f;

    bool damageAnimating = false;
    float damageAnimElapsedSeconds = 0.0f;
    float damageAnimDurationSeconds = 0.10f;

    // Optional health bar (in-world).
    bool hasHealthBar = false;
    Bar healthBar;

    // Allows specific subclasses (e.g., drones/traps) to opt into same-group collision response.
    virtual bool sameGroupCollision(const GameObject& other) const {
        (void)other;
        return false;
    }

    virtual float getScoreReward() const {
        return 0.0f;
    }

    int getId() const {
        return m_id;
    }

    bool isDead() const {
        return maxHealth > 0.0f && health <= 0.0f;
    }

    bool isInvisible() const {
        return opacity < 0.001f;
    }

    float getRadiusScaled() const {
        return radius * scale;
    }

    QRectF getBoundingBox() const {
        float r = getRadiusScaled();
        return QRectF(position.x() - r, position.y() - r, 2 * r, 2 * r);
    }

    bool checkCollision(const GameObject& other) const {
        const float touchingDistance = getRadiusScaled() + other.getRadiusScaled();
        const QPointF d = position - other.position;
        return (d.x() * d.x() + d.y() * d.y()) < (touchingDistance * touchingDistance);
    }

    void addForce(const float &acceleration, const float &directionRadians) {
        lastAcceleration = acceleration;
        velocity += QPointF(std::cos(directionRadians), std::sin(directionRadians)) * acceleration;
    }

    void addForceVec(const QPointF &v) {
        velocity += v;
    }

    void restrictPosition(float deltaTime) {
        float r = getRadiusScaled();
        float t = ((1.0f - lastAcceleration) * 10.0f) * deltaTime;
        if (position.x() > (Settings::arenaWidth + Settings::arenaBoundsPad) - r)
            position.setX((1 - t) * position.x() + t * ((Settings::arenaWidth + Settings::arenaBoundsPad) - r));
        if (position.x() < r - Settings::arenaBoundsPad)
            position.setX((1 - t) * position.x() + t * (r - Settings::arenaBoundsPad));
        if (position.y() > (Settings::arenaHeight + Settings::arenaBoundsPad) - r)
            position.setY((1 - t) * position.y() + t * ((Settings::arenaHeight + Settings::arenaBoundsPad) - r));
        if (position.y() < r - Settings::arenaBoundsPad)
            position.setY((1 - t) * position.y() + t * (r - Settings::arenaBoundsPad));

        // bouncy
        // if (position.x() < 0) {
        //     position.setX(0);
        //     velocity.setX(std::abs(velocity.x() * absorptionFactor));
        // }
        // if (position.x() > Settings::arenaWidth) {
        //     position.setX(Settings::arenaWidth);
        //     velocity.setX(-std::abs(velocity.x() * absorptionFactor));
        // }
        // if (position.y() < 0) {
        //     position.setY(0);
        //     velocity.setY(std::abs(velocity.y() * absorptionFactor));
        // }
        // if (position.y() > Settings::arenaHeight) {
        //     position.setY(Settings::arenaHeight);
        //     velocity.setY(-std::abs(velocity.y() * absorptionFactor));
        // }
    }

    virtual void update(float deltaTime) {
        if (spawnProtection) {
            spawnProtectionElapsedSeconds += std::max(0.0f, deltaTime);
        } else {
            spawnProtectionElapsedSeconds = 0.0f;
        }

        if (damageAnimElapsedSeconds < damageAnimDurationSeconds)
            damageAnimElapsedSeconds = std::min(damageAnimDurationSeconds, damageAnimElapsedSeconds + std::max(0.0f, deltaTime));
        else if (damageAnimating)
            damageAnimating = false;

        if (hasHealthBar && maxHealth > 0.0f) {
            healthBar.setWidth(getRadiusScaled() * 2.0f);
            healthBar.update(deltaTime, health / maxHealth);
            if (health >= maxHealth) {
                healthBar.setHidden(true);
            }
        }

        position += velocity;
        velocity *= friction;

        if (keepInArena)
            restrictPosition(deltaTime);

        if (!deathAnimating && isDead())
            triggerDelete();

        if (deathAnimating) {
            deathAnimElapsedSeconds = std::min(deathAnimDurationSeconds, deathAnimElapsedSeconds + std::max(0.0f, deltaTime));
            const float t = (deathAnimDurationSeconds > 1e-6f) ? (deathAnimElapsedSeconds / deathAnimDurationSeconds) : 1.0f;

            constexpr float kFinalScaleMultiplier = 1.6f;
            scale = deathAnimStartScale * (1.0f + (kFinalScaleMultiplier - 1.0f) * t);
            opacity = deathAnimStartOpacity * (1.0f - t);

            if (deathAnimElapsedSeconds >= deathAnimDurationSeconds)
                pendingDelete = true;
        }
    }

    void initHealthBar() {
        hasHealthBar = true;
        const QColor fill = Settings::palette.value("HEALTH_BAR", QColor(133, 227, 125));
        healthBar = Bar(getRadiusScaled() * 2.0f, 15.0f, 3.0f, fill, Settings::globalStrokeColor, 0.15f, 1.0f);
        healthBar.forceHidden(true);
    }

    void receiveKnockback(const GameObject& other) {
        if (isDead() || other.isDead())
            return;

        // Scales push based on friction.
        // https://www.desmos.com/calculator/p9tyewb18m and https://www.desmos.com/calculator/julb0fev5e
        const float scaleFactor = (-1.96251f * friction + 1.96557f);
        const float knockbackMagnitude = absorptionFactor * other.pushFactor * scaleFactor;

        float dx = static_cast<float>(position.x() - other.position.x());
        float dy = static_cast<float>(position.y() - other.position.y());

        if (dx == 0.0f && dy == 0.0f) {
            // If objects are perfectly stacked, nudge so the angle is well-defined.
            dx = 0.1f;
            dy = 0.1f;
        }

        const float angle = std::atan2(dy, dx);
        addForce(knockbackMagnitude, angle);
    }

    static void receiveKnockback(GameObject& a, GameObject& b) {
        if (a.isDead() || b.isDead())
            return;

        // If different group OR same group but neither have noInternalCollision OR same-group override.
        if (a.group != b.group || (!a.noInternalCollision && !b.noInternalCollision) || a.sameGroupCollision(b)) {
            a.receiveKnockback(b);
            b.receiveKnockback(a);
        }
    }

    virtual void updateVictim(GameObject& victim) {
        (void)victim;
    }

    void receiveDamage(float dmg) {
        if (isDead() || spawnProtection)
            return;
        if (dmg <= 0.0f)
            return;

        damageAnimating = true;
        damageAnimElapsedSeconds = 0.0f;

        health -= dmg;

        healthBar.forceHidden(false);

        if (health <= 0.0f) {
            health = 0.0f;
            triggerDelete();
        }
    }

    void triggerDelete() {
        if (deathAnimating || pendingDelete)
            return;
        deathAnimating = true;
        deathAnimElapsedSeconds = 0.0f;
        deathAnimStartScale = scale;
        deathAnimStartOpacity = opacity;
    }

    QColor getDamageLerpColor(const QColor& col) const {
        auto lerp = [](const QColor& a, const QColor& b, float t) -> QColor {
            t = std::clamp(t, 0.0f, 1.0f);
            const int r = static_cast<int>(std::lround((1.0f - t) * a.red() + t * b.red()));
            const int g = static_cast<int>(std::lround((1.0f - t) * a.green() + t * b.green()));
            const int bl = static_cast<int>(std::lround((1.0f - t) * a.blue() + t * b.blue()));
            const int al = static_cast<int>(std::lround((1.0f - t) * a.alpha() + t * b.alpha()));
            return QColor(r, g, bl, al);
        };

        QColor out = col;

        if (damageAnimating) {
            const float rem = 1.0f - (damageAnimElapsedSeconds / std::max(1e-6f, damageAnimDurationSeconds));

            const QColor red = Settings::palette.value("RED", QColor(241, 78, 84));
            const QColor white = QColor(255, 255, 255);

            if (rem >= (10.0f / 12.0f))
                out = lerp(col, white, 0.85f * rem);
            else
                out = lerp(col, red, rem);
        }

        if (spawnProtection) {
            const QColor white = QColor(255, 255, 255);
            const float t = 0.5f * (1.0f + std::sin(spawnProtectionElapsedSeconds * 2.0f * std::numbers::pi_v<float> * 4.0f));
            out = lerp(out, white, 0.25f + 0.60f * t);
        }

        return out;
    }

    static void receiveDamage(GameObject& a, GameObject& b) {
        if (a.isDead() || b.isDead())
            return;

        // Different groups damage each other.
        if (a.group == b.group)
            return;

        if (a.spawnProtection || b.spawnProtection)
            return;

        const float aDamage = a.damage * b.damageFactor;
        const float bDamage = b.damage * a.damageFactor;

        if (aDamage <= 0.0f && bDamage <= 0.0f)
            return;

        // Overkill scaling: if one side would die, scale both damages so it dies exactly.
        if (a.maxHealth > 0.0f && bDamage > a.health && bDamage > 0.0f) {
            const float scaleDown = a.health / bDamage;
            a.receiveDamage(scaleDown * bDamage);
            b.receiveDamage(scaleDown * aDamage);
        } else if (b.maxHealth > 0.0f && aDamage > b.health && aDamage > 0.0f) {
            const float scaleDown = b.health / aDamage;
            b.receiveDamage(scaleDown * aDamage);
            a.receiveDamage(scaleDown * bDamage);
        } else {
            a.receiveDamage(bDamage);
            b.receiveDamage(aDamage);
        }

        if (a.isDead())
            b.updateVictim(a);
        if (b.isDead())
            a.updateVictim(b);
    }

    virtual void draw(QPainter* painter) const {
        painter->setOpacity(opacity);
        painter->setBrush(getDamageLerpColor(fillColor));
        painter->setPen(QPen(getDamageLerpColor(strokeColor), 5));

        switch (geometry) {
            case Geometry::Circle: {
                float r = getRadiusScaled();
                painter->drawEllipse(position, r, r);
                break;
            }
            case Geometry::Rect: {
                painter->save();
                painter->translate(position);
                painter->rotate(rotationRadians * 180.0f / std::numbers::pi_v<float>);
                const QSizeF s = rectSize * scale;
                painter->drawRect(QRectF(-s.width() * 0.5, -s.height() * 0.5, s.width(), s.height()));
                painter->restore();
                break;
            }
            case Geometry::Polygon: {
                painter->save();
                painter->translate(position);
                painter->rotate(rotationRadians * 180.0f / std::numbers::pi_v<float>);
                QPolygonF poly = localPolygon;
                if (!poly.isEmpty()) {
                    // Apply uniform scaling around the origin.
                    for (auto& p : poly)
                        p *= scale;
                    painter->drawPolygon(poly);
                }
                painter->restore();
                break;
            }
        }

        if (hasHealthBar && maxHealth > 0.0f) {
            const float pad = 8.0f;
            const float w = getRadiusScaled() * 2.0f;
            const float h = 15.0f;
            const QPointF topLeft(position.x() - w * 0.5f, position.y() + getRadiusScaled() + h - pad);
            healthBar.draw(painter, topLeft);
        }
    }

private:
    int m_id = 0;
    static inline int totalIds = 0;
};

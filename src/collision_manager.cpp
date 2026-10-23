#include "collision_manager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "game_object.hpp"

namespace {

std::uint64_t pairKey(int a, int b) {
    const std::uint32_t lo = static_cast<std::uint32_t>(std::min(a, b));
    const std::uint32_t hi = static_cast<std::uint32_t>(std::max(a, b));
    return (static_cast<std::uint64_t>(lo) << 32) | static_cast<std::uint64_t>(hi);
}

} // namespace

int CollisionManager::sectorX(float x) const {
    const float s = std::max(1e-6f, m_sectorSize);
    return static_cast<int>((x - m_minX) / s);
}

int CollisionManager::sectorY(float y) const {
    const float s = std::max(1e-6f, m_sectorSize);
    return static_cast<int>((y - m_minY) / s);
}

void CollisionManager::updateCollision(const std::vector<GameObject*>& objects) {
    m_groups.clear();
    m_collidedPairs.clear();

    if (objects.empty())
        return;

    float minX = std::numeric_limits<float>::infinity();
    float minY = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    float maxY = -std::numeric_limits<float>::infinity();

    // Compute world bounds across all objects.
    for (const auto* obj : objects) {
        if (!obj)
            continue;
        const QRectF bb = obj->getBoundingBox();
        minX = std::min(minX, static_cast<float>(bb.x()));
        minY = std::min(minY, static_cast<float>(bb.y()));
        maxX = std::max(maxX, static_cast<float>(bb.x() + bb.width()));
        maxY = std::max(maxY, static_cast<float>(bb.y() + bb.height()));
    }

    if (!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(maxX) || !std::isfinite(maxY))
        return;

    m_minX = minX;
    m_minY = minY;

    const float s = std::max(1e-6f, m_sectorSize);
    m_sectorsX = static_cast<int>((maxX - minX) / s) + 2;

    // Insert objects into all sectors their bounding box overlaps.
    for (auto* obj : objects) {
        if (!obj)
            continue;

        const QRectF bb = obj->getBoundingBox();
        const int xiMin = sectorX(static_cast<float>(bb.x()));
        const int xiMax = sectorX(static_cast<float>(bb.x() + bb.width()));
        const int yiMin = sectorY(static_cast<float>(bb.y()));
        const int yiMax = sectorY(static_cast<float>(bb.y() + bb.height()));

        for (int xi = xiMin; xi <= xiMax; xi++) {
            for (int yi = yiMin; yi <= yiMax; yi++) {
                const int hash = xi + m_sectorsX * yi;
                m_groups[hash].push_back(obj);
            }
        }
    }

    // Test within each group.
    for (auto& [hash, group] : m_groups) {
        (void)hash;
        if (group.size() <= 1)
            continue;

        for (size_t i = 0; i < group.size(); i++) {
            for (size_t j = i + 1; j < group.size(); j++) {
                GameObject* a = group[i];
                GameObject* b = group[j];
                if (!a || !b)
                    continue;
                if (a == b)
                    continue;

                const std::uint64_t key = pairKey(a->getId(), b->getId());
                if (m_collidedPairs.contains(key))
                    continue;

                if (a->checkCollision(*b)) {
                    GameObject::receiveKnockback(*a, *b);
                    GameObject::receiveDamage(*a, *b);
                    m_collidedPairs.insert(key);
                }
            }
        }
    }
}

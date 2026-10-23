#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <QRectF>

class GameObject;

class CollisionManager {
public:
    void setSectorSize(float sectorSize) {
        m_sectorSize = sectorSize;
    }

    float sectorSize() const {
        return m_sectorSize;
    }

    void updateCollision(const std::vector<GameObject*>& objects);

private:
    int sectorX(float x) const;
    int sectorY(float y) const;

private:
    float m_sectorSize = 150.0f;

    int m_sectorsX = 0;
    float m_minX = 0.0f;
    float m_minY = 0.0f;

    std::unordered_map<int, std::vector<GameObject*>> m_groups;
    std::unordered_set<std::uint64_t> m_collidedPairs;
};


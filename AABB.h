#pragma once
#include <glm/glm.hpp>
#include <vector> 



class AABB {
private:
    glm::vec3 minPoint;
    glm::vec3 maxPoint;

public:
    AABB() : minPoint(0.0f), maxPoint(0.0f) {}  // 默認構造函數
    AABB(const glm::vec3& min, const glm::vec3& max) : minPoint(min), maxPoint(max) {}    glm::vec3 GetMin() const { return minPoint; }
    glm::vec3 GetMax() const { return maxPoint; }

    // 球體與球體的碰撞檢測
    static bool SphereToSphere(const glm::vec3& pos1, float radius1, const glm::vec3& pos2, float radius2) {
        float distance = glm::length(pos1 - pos2);
        return distance < (radius1 + radius2);
    }

    // 計算距離 AABB 最近的點
    glm::vec3 ClosestPoint(const glm::vec3& point) const {
        return glm::vec3(
            glm::clamp(point.x, minPoint.x, maxPoint.x),
            glm::clamp(point.y, minPoint.y, maxPoint.y),
            glm::clamp(point.z, minPoint.z, maxPoint.z)
        );
    }

};
#pragma once
#include <glm/glm.hpp>
// BoundingSphere 類定義
class BoundingSphere {
public:
    glm::vec3 center;
    float radius;
    
    BoundingSphere() : center(0.0f), radius(0.0f) {}
    BoundingSphere(const glm::vec3& c, float r) : center(c), radius(r) {}
    
    // 設置球體的中心點
    void SetCenter(const glm::vec3& newCenter) {
        center = newCenter;
    }
};
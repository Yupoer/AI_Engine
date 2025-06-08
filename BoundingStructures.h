#pragma once

#include <glm/glm.hpp>

struct BoundingSphere {
    glm::vec3 center;
    float radius;

    BoundingSphere(const glm::vec3& c = glm::vec3(0.0f), float r = 0.0f) : center(c), radius(r) {}
};
// File: AABB.h (正確的版本)
#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "BoundingStructures.h" // For BoundingSphere

class AABB {
private:
    glm::vec3 minPoint;
    glm::vec3 maxPoint;
    static bool showAABBGlobal; // 或者你之前使用的名稱 showAABB

public:
    AABB(const glm::vec3& min = glm::vec3(0.0f), const glm::vec3& max = glm::vec3(0.0f));
    AABB(const BoundingSphere& sphere);

    glm::vec3 GetMin() const;
    glm::vec3 GetMax() const;

    glm::vec3 ClosestPoint(const glm::vec3& point) const;
    bool Intersects(const AABB& other) const;
    AABB Transform(const glm::mat4& transformMatrix) const;

    static bool SphereToAABB(const BoundingSphere& sphere, const AABB& aabb);
    static bool SphereToAABB(const glm::vec3& sphereCenter, float sphereRadius, const AABB& aabb);
    static bool SphereToSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2);
    static bool SphereToSphere(const glm::vec3& center1, float radius1, const glm::vec3& center2, float radius2);

    static void DrawAABB(const AABB& aabb, GLuint shaderProgram, GLuint VAO, bool showCollisionVolumes);
    static void DrawWireSphere(const BoundingSphere& sphere, GLuint shaderProgram, GLuint VAO, bool showCollisionVolumes);

    static void SetShowCollisionVolumes(bool show); // 或者你之前使用的 SetShowAABB
    static bool GetShowCollisionVolumes();      // 或者你之前使用的 GetShowAABB
};
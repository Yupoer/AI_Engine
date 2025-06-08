// File: AABB.cpp
#include "AABB.h"
#include "BoundingStructures.h" // Just in case, though AABB.h includes it
#include <algorithm> // For glm::min/max if not covered by glm include
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cfloat> // For FLT_MAX

bool AABB::showAABBGlobal = false; // Definition for the static member

AABB::AABB(const glm::vec3& min, const glm::vec3& max) : minPoint(min), maxPoint(max) {}

// Constructor to create an AABB that encloses a sphere
AABB::AABB(const BoundingSphere& sphere)
    : minPoint(sphere.center - glm::vec3(sphere.radius)),
      maxPoint(sphere.center + glm::vec3(sphere.radius)) {}

glm::vec3 AABB::GetMin() const { return minPoint; }
glm::vec3 AABB::GetMax() const { return maxPoint; }

glm::vec3 AABB::ClosestPoint(const glm::vec3& point) const {
    glm::vec3 result;
    result.x = std::max(minPoint.x, std::min(point.x, maxPoint.x)); // glm::clamp equivalent
    result.y = std::max(minPoint.y, std::min(point.y, maxPoint.y));
    result.z = std::max(minPoint.z, std::min(point.z, maxPoint.z));
    return result;
}

bool AABB::Intersects(const AABB& other) const {
    if (maxPoint.x < other.minPoint.x || minPoint.x > other.maxPoint.x) return false;
    if (maxPoint.y < other.minPoint.y || minPoint.y > other.maxPoint.y) return false;
    if (maxPoint.z < other.minPoint.z || minPoint.z > other.maxPoint.z) return false;
    return true;
}

AABB AABB::Transform(const glm::mat4& transformMatrix) const {
    // Transform all 8 corners of the AABB and find the new min/max
    glm::vec3 corners[8] = {
        glm::vec3(minPoint.x, minPoint.y, minPoint.z),
        glm::vec3(maxPoint.x, minPoint.y, minPoint.z),
        glm::vec3(minPoint.x, maxPoint.y, minPoint.z),
        glm::vec3(minPoint.x, minPoint.y, maxPoint.z),
        glm::vec3(maxPoint.x, maxPoint.y, minPoint.z),
        glm::vec3(minPoint.x, maxPoint.y, maxPoint.z),
        glm::vec3(maxPoint.x, minPoint.y, maxPoint.z),
        glm::vec3(maxPoint.x, maxPoint.y, maxPoint.z),
    };

    glm::vec3 newMin(FLT_MAX);
    glm::vec3 newMax(-FLT_MAX);

    for (int i = 0; i < 8; ++i) {
        glm::vec4 transformedCorner = transformMatrix * glm::vec4(corners[i], 1.0f);
        newMin.x = std::min(newMin.x, transformedCorner.x);
        newMin.y = std::min(newMin.y, transformedCorner.y);
        newMin.z = std::min(newMin.z, transformedCorner.z);
        newMax.x = std::max(newMax.x, transformedCorner.x);
        newMax.y = std::max(newMax.y, transformedCorner.y);
        newMax.z = std::max(newMax.z, transformedCorner.z);
    }
    return AABB(newMin, newMax);
}


// Static collision functions
bool AABB::SphereToAABB(const BoundingSphere& sphere, const AABB& aabb) {
    glm::vec3 closestPoint = aabb.ClosestPoint(sphere.center);
    float distSq = glm::dot(sphere.center - closestPoint, sphere.center - closestPoint);
    return distSq <= (sphere.radius * sphere.radius);
}

bool AABB::SphereToAABB(const glm::vec3& sphereCenter, float sphereRadius, const AABB& aabb) {
    glm::vec3 closestPoint = aabb.ClosestPoint(sphereCenter);
    float distSq = glm::dot(sphereCenter - closestPoint, sphereCenter - closestPoint);
    return distSq <= (sphereRadius * sphereRadius);
}

bool AABB::SphereToSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2) {
    float distSq = glm::dot(sphere2.center - sphere1.center, sphere2.center - sphere1.center);
    float radiiSum = sphere1.radius + sphere2.radius;
    return distSq <= (radiiSum * radiiSum);
}

bool AABB::SphereToSphere(const glm::vec3& center1, float radius1, const glm::vec3& center2, float radius2) {
    float distSq = glm::dot(center2 - center1, center2 - center1);
    float radiiSum = radius1 + radius2;
    return distSq <= (radiiSum * radiiSum);
}


// Static drawing functions
void AABB::DrawAABB(const AABB& aabb, GLuint shaderProgram, GLuint VAO, bool showCollisionVolumes) {
    if (!showCollisionVolumes) return;

    glm::mat4 model = glm::mat4(1.0f);
    // Center the model at the AABB's center and scale it to the AABB's dimensions
    model = glm::translate(model, (aabb.minPoint + aabb.maxPoint) * 0.5f);
    model = glm::scale(model, (aabb.maxPoint - aabb.minPoint));

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelMat"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(shaderProgram, "objColor"), 1.0f, 0.0f, 0.0f); // Red for AABB
    glUniform1i(glGetUniformLocation(shaderProgram, "isAABB"), 1); // Or a more generic "useWireframeColor"

    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void AABB::DrawWireSphere(const BoundingSphere& sphere, GLuint shaderProgram, GLuint VAO, bool showCollisionVolumes) {
    if (!showCollisionVolumes) return;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, sphere.center);
    model = glm::scale(model, glm::vec3(sphere.radius));
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelMat"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(shaderProgram, "objColor"), 0.0f, 1.0f, 0.0f); // Green for BoundingSphere
    glUniform1i(glGetUniformLocation(shaderProgram, "isAABB"), 1); // Or a generic wireframe uniform

    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0); // Placeholder: Adjust for your sphere VAO
    glBindVertexArray(0);
}


void AABB::SetShowCollisionVolumes(bool show) {
    showAABBGlobal = show;
}

bool AABB::GetShowCollisionVolumes() {
    return showAABBGlobal;
}
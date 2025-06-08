// File: OBB.cpp
#include "OBB.h"
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr

OBB::OBB(const glm::vec3& c, const glm::mat3& o, const glm::vec3& h)
    : center(c), orientation(o), halfExtents(h) {}

glm::vec3 OBB::getCenter() const { return center; }
glm::mat3 OBB::getOrientation() const { return orientation; }
glm::vec3 OBB::getHalfExtents() const { return halfExtents; }

void OBB::setCenter(const glm::vec3& c) { center = c; }
void OBB::setOrientation(const glm::mat3& o) { orientation = o; }
void OBB::setHalfExtents(const glm::vec3& h) { halfExtents = h; }

// Definition for DrawOBB
void OBB::DrawOBB(const OBB& obb, GLuint shaderProgram, GLuint VAO, bool showCollisionVolumes) {
    if (!showCollisionVolumes) return;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, obb.getCenter());
    model *= glm::mat4(obb.getOrientation()); // Apply orientation
    model = glm::scale(model, obb.getHalfExtents() * 2.0f); // Scale by full extents

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelMat"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(shaderProgram, "objColor"), 0.0f, 0.0f, 1.0f); // Blue for OBB
    glUniform1i(glGetUniformLocation(shaderProgram, "isAABB"), 1); // Assuming this uniform is generic for wireframe

    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
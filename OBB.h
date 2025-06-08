// File: OBB.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <GL/glew.h> 

class OBB {
private:
    glm::vec3 center;
    glm::mat3 orientation;
    glm::vec3 halfExtents;

public:
    OBB(const glm::vec3& c = glm::vec3(0.0f),
        const glm::mat3& o = glm::mat3(1.0f),
        const glm::vec3& h = glm::vec3(0.0f));

    glm::vec3 getCenter() const;
    glm::mat3 getOrientation() const;
    glm::vec3 getHalfExtents() const;

    void setCenter(const glm::vec3& c);
    void setOrientation(const glm::mat3& o);
    void setHalfExtents(const glm::vec3& h);

    // Consider moving drawing logic to a dedicated Renderer class
    // or passing the 'show' status directly.
    static void DrawOBB(const OBB& obb, GLuint shaderProgram, GLuint VAO, bool showCollisionVolumes);
};
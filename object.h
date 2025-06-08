// File: object.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "BoundingStructures.h" // For BoundingSphere
#include "AABB.h" 
#include "OBB.h"                // For OBB class

class Object {
public:
    // --- Members ---
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 velocity;
    glm::vec3 angularVelocity;
    glm::vec3 force;
    glm::vec3 torque;

    // Local space properties
    glm::vec3 minLocal; // For non-sphere objects, local AABB min
    glm::vec3 maxLocal; // For non-sphere objects, local AABB max
    glm::vec3 centerLocal; // For non-sphere objects, local center

    glm::vec3 centerOfMass; // Relative to local origin
    glm::vec3 initPosition;
    glm::quat initRotation;

    // Bounding Volumes (World Space)
    BoundingSphere sphereWorld; // Always updated, primarily used if isSphere is true
    OBB obbWorld;             // Always updated, primarily used if isSphere is false
    AABB aabbWorld;            // Always updated, for broad-phase or grid, and room collision

    // Sphere specific properties
    bool isSphere;
    float sphereRadius; // Only if isSphere is true

    // Physical properties
    float mass;
    glm::mat3 inertiaTensor; // Local space inertia tensor
    float frictionCoeff;
    float restitution;
    float damping;

    // --- Constructors ---
    // Constructor for box-like objects
    Object(const glm::vec3& localMin, const glm::vec3& localMax,
           const glm::vec3& startPos,
           float mass = 1.0f, float friction = 0.5f, float restitution = 0.5f, float damping = 0.98f);

    // Constructor for sphere objects
    Object(float radius, const glm::vec3& startPos, // Center of sphere is its position
           float mass = 1.0f, float friction = 0.5f, float restitution = 0.5f, float damping = 0.98f);

    // --- Methods ---
    void translate(const glm::vec3& delta);
    void rotate(const glm::quat& q);

    const AABB& GetAABB() const { return aabbWorld; }
    const OBB& GetOBB() const { return obbWorld; }
    const BoundingSphere& GetBoundingSphere() const { return sphereWorld; }
    
    void updateAABB(); // Updates aabbWorld based on current state
    void updateOBB();  // Updates obbWorld based on current state
    void updateBoundingSphere(); // Updates sphereWorld based on current state
    void reset();

    // Physics related
    void applyForce(const glm::vec3& appliedForce, const glm::vec3& applyPointWorld); // Point is in world space
    void applyImpulse(const glm::vec3& impulse, const glm::vec3& applyPointWorld); // Point is in world space. Impulse is a direct change in momentum.
    void updatePhysics(float deltaTime);
    glm::vec3 getWorldCenterOfMass() const;
    glm::mat3 getWorldInertiaTensor() const; // Inertia tensor in world space

    // Collision handling (simplified, you'll expand this)
    void handleCollision(Object& other, const glm::vec3& contactPoint, const glm::vec3& normal);


    // --- Getters ---
    glm::vec3 GetPosition() const { return position; }
    glm::quat GetRotation() const { return rotation; }
    glm::vec3 GetVelocity() const { return velocity; }
    // ... other getters as needed ...

    bool IsSphereObject() const { return isSphere; } // Renamed for clarity
    float GetSphereRadius() const; // Only valid if IsSphereObject() is true

    // --- Setters ---
    void SetPosition(const glm::vec3& pos);
    void SetRotation(const glm::quat& rot);
    void SetVelocity(const glm::vec3& vel) { velocity = vel; }
};
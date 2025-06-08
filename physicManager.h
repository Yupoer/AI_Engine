// File: physicManager.h
#pragma once
#include <vector>
#include "object.h" // object.h now includes AABB.h, OBB.h, BoundingStructures.h    
#include "AABB.h" // For the roomAABB parameter type

class PhysicManager {
public:
    bool pausePhysics = false;
    float gravityAcceleration = 9.81f; // More descriptive name
    float angularDragCoefficient;

    PhysicManager(float g = 9.81f);
    void update(std::vector<Object*>& objects, const AABB& roomAABB, float deltaTime);
    void applyGravity(Object& obj) const; // Helper for gravity
};
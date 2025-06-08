// File: object.cpp
#include "object.h"
#include <cfloat>     // For FLT_MAX
#include <algorithm>  // For std::min/max

// Constructor for box-like objects
Object::Object(const glm::vec3& localMin, const glm::vec3& localMax,
               const glm::vec3& startPos,
               float m, float f, float r, float d)
    : position(startPos), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      velocity(0.0f), angularVelocity(0.0f), force(0.0f), torque(0.0f),
      minLocal(localMin), maxLocal(localMax),
      initPosition(startPos), initRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      isSphere(false), sphereRadius(0.0f), // Not a sphere by default
      mass(m), frictionCoeff(f), restitution(r), damping(d) {

    centerLocal = (minLocal + maxLocal) * 0.5f;
    centerOfMass = centerLocal; // Assuming uniform density for now

    // Inertia tensor for a cuboid
    float width = maxLocal.x - minLocal.x;
    float height = maxLocal.y - minLocal.y;
    float depth = maxLocal.z - minLocal.z;
    float Ix = (1.0f / 12.0f) * mass * (height * height + depth * depth);
    float Iy = (1.0f / 12.0f) * mass * (width * width + depth * depth);
    float Iz = (1.0f / 12.0f) * mass * (width * width + height * height);
    inertiaTensor = glm::mat3(Ix, 0.0f, 0.0f,
                              0.0f, Iy, 0.0f,
                              0.0f, 0.0f, Iz);

    updateAABB();
    updateOBB();
    updateBoundingSphere(); // Calculate a sphere that encloses the box
}

// Constructor for sphere objects
Object::Object(float radius, const glm::vec3& startPos,
               float m, float f, float r, float d)
    : position(startPos), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), // Spheres don't typically rotate visually unless textured
      velocity(0.0f), angularVelocity(0.0f), force(0.0f), torque(0.0f),
      minLocal(-radius), maxLocal(radius), // Define a local bounding box for consistency if needed
      initPosition(startPos), initRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      isSphere(true), sphereRadius(radius),
      mass(m), frictionCoeff(f), restitution(r), damping(d) {

    centerLocal = glm::vec3(0.0f); // Sphere's local center is at its origin
    centerOfMass = glm::vec3(0.0f);

    // Inertia tensor for a solid sphere
    float I = (2.0f / 5.0f) * mass * sphereRadius * sphereRadius;
    inertiaTensor = glm::mat3(I, 0.0f, 0.0f,
                              0.0f, I, 0.0f,
                              0.0f, 0.0f, I);
    updateAABB();
    updateOBB();
    updateBoundingSphere();
}

void Object::updateAABB() {
    if (isSphere) {
        aabbWorld = AABB(sphereWorld); // AABB enclosing the sphereWorld
    } else {
        // Transform OBB corners to world space and find min/max
        glm::vec3 obb_corners[8];
        glm::vec3 obbCenter = obbWorld.getCenter();
        glm::mat3 obbOrient = obbWorld.getOrientation();
        glm::vec3 obbExt = obbWorld.getHalfExtents();

        obb_corners[0] = obbCenter + obbOrient * glm::vec3(-obbExt.x, -obbExt.y, -obbExt.z);
        obb_corners[1] = obbCenter + obbOrient * glm::vec3( obbExt.x, -obbExt.y, -obbExt.z);
        obb_corners[2] = obbCenter + obbOrient * glm::vec3(-obbExt.x,  obbExt.y, -obbExt.z);
        obb_corners[3] = obbCenter + obbOrient * glm::vec3( obbExt.x,  obbExt.y, -obbExt.z);
        obb_corners[4] = obbCenter + obbOrient * glm::vec3(-obbExt.x, -obbExt.y,  obbExt.z);
        obb_corners[5] = obbCenter + obbOrient * glm::vec3( obbExt.x, -obbExt.y,  obbExt.z);
        obb_corners[6] = obbCenter + obbOrient * glm::vec3(-obbExt.x,  obbExt.y,  obbExt.z);
        obb_corners[7] = obbCenter + obbOrient * glm::vec3( obbExt.x,  obbExt.y,  obbExt.z);
        
        glm::vec3 minP(FLT_MAX);
        glm::vec3 maxP(-FLT_MAX);

        for (int i = 0; i < 8; ++i) {
            minP.x = std::min(minP.x, obb_corners[i].x);
            minP.y = std::min(minP.y, obb_corners[i].y);
            minP.z = std::min(minP.z, obb_corners[i].z);
            maxP.x = std::max(maxP.x, obb_corners[i].x);
            maxP.y = std::max(maxP.y, obb_corners[i].y);
            maxP.z = std::max(maxP.z, obb_corners[i].z);
        }
        aabbWorld = AABB(minP, maxP);
    }
}

void Object::updateOBB() {
    // OBB center is the world position + rotated local center
    obbWorld.setCenter(position + (rotation * (centerLocal - glm::vec3(0,0,0))) ); // Assuming centerLocal is relative to object's own origin
    obbWorld.setOrientation(glm::mat3_cast(rotation));
    if (isSphere) {
        obbWorld.setHalfExtents(glm::vec3(sphereRadius));
    } else {
        // Half extents are based on localMin/localMax transformed by rotation (but extents are scalar)
        glm::vec3 localHalfExtents = (maxLocal - minLocal) * 0.5f;
        obbWorld.setHalfExtents(localHalfExtents);
    }
}


void Object::updateBoundingSphere() {
    sphereWorld.center = position; // For spheres, center is position
    if (isSphere) {
        sphereWorld.radius = sphereRadius;
    } else {
        // For non-spheres, calculate a sphere that encloses the OBB
        // A simple bounding sphere can be centered at the OBB's center
        // with radius equal to the distance to the furthest OBB corner.
        sphereWorld.center = obbWorld.getCenter();
        glm::vec3 ext = obbWorld.getHalfExtents();
        sphereWorld.radius = glm::length(ext); // Distance from center to corner of the OBB's local aligned box
    }
}

void Object::translate(const glm::vec3& delta) {
    position += delta;
    // Object has moved, bounding volumes need to follow
    updateOBB(); // OBB moves with position
    updateAABB(); // AABB recalculates based on new OBB/Sphere
    updateBoundingSphere(); // Sphere moves with position
}

void Object::rotate(const glm::quat& q) {
    rotation = glm::normalize(q * rotation);
    // Object has rotated, OBB orientation changes, AABB and BoundingSphere might change
    updateOBB();
    updateAABB();
    updateBoundingSphere();
}

void Object::reset() {
    position = initPosition;
    rotation = initRotation;
    velocity = glm::vec3(0.0f);
    angularVelocity = glm::vec3(0.0f);
    force = glm::vec3(0.0f);
    torque = glm::vec3(0.0f);
    updateOBB();
    updateAABB();
    updateBoundingSphere();
}

void Object::applyForce(const glm::vec3& appliedForce, const glm::vec3& applyPointWorld) {
    this->force += appliedForce;
    glm::vec3 r = applyPointWorld - getWorldCenterOfMass(); // r is vector from CoM to point of application
    this->torque += glm::cross(r, appliedForce);
}

void Object::updatePhysics(float deltaTime) {
    if (mass <= 0.0f) return; // Infinite mass objects don't move

    // Linear motion
    glm::vec3 acceleration = force / mass;
    velocity += acceleration * deltaTime;
    position += velocity * deltaTime;

    // Angular motion (only for non-spheres for now, or if spheres can have angular velocity)
    if (!isSphere || glm::dot(angularVelocity, angularVelocity) > 0.0001f ) { // Allow spheres to have angular velocity if set
        glm::mat3 invInertiaTensorWorld = getWorldInertiaTensor(); // This should be inverse
        // Check if matrix is invertible
        if (glm::determinant(invInertiaTensorWorld) != 0.0f) {
             invInertiaTensorWorld = glm::inverse(invInertiaTensorWorld);
             glm::vec3 angularAcceleration = invInertiaTensorWorld * torque;
             angularVelocity += angularAcceleration * deltaTime;
        }
    }


    // Apply damping
    velocity *= std::pow(damping, deltaTime); // Frame-rate independent damping
    angularVelocity *= std::pow(damping, deltaTime);


    // Update rotation from angular velocity
    if (glm::dot(angularVelocity, angularVelocity) > 0.00001f) {
        glm::quat deltaRotation = glm::quat(0.0f, angularVelocity.x * deltaTime, angularVelocity.y * deltaTime, angularVelocity.z * deltaTime) * 0.5f;
        rotation = glm::normalize(rotation + deltaRotation * rotation); // Simplified integration
    }


    // Clear forces for next frame
    force = glm::vec3(0.0f);
    torque = glm::vec3(0.0f);

    // Gravity (example, could be part of forces added externally)
    // applyForce(glm::vec3(0.0f, -mass * 9.81f, 0.0f), getWorldCenterOfMass()); // This would be applied each frame by PhysicManager perhaps

    // Update all bounding volumes after position/rotation change
    updateOBB();
    updateAABB();
    updateBoundingSphere();
}


glm::vec3 Object::getWorldCenterOfMass() const {
    // Convert local centerOfMass to world space
    return position + (rotation * centerOfMass);
}

glm::mat3 Object::getWorldInertiaTensor() const {
    glm::mat3 R = glm::mat3_cast(rotation);
    return R * inertiaTensor * glm::transpose(R);
}

void Object::handleCollision(Object& other, const glm::vec3& contactPoint, const glm::vec3& normal) {
    // 計算質心世界座標
    glm::vec3 cmA = getWorldCenterOfMass();
    glm::vec3 cmB = other.getWorldCenterOfMass();

    // 計算 r_A 和 r_B
    glm::vec3 rA = contactPoint - cmA;
    glm::vec3 rB = contactPoint - cmB;

    // 計算質心速度（假設 velocity 是質心速度）
    glm::vec3 v_cmA = velocity;
    glm::vec3 v_cmB = other.velocity;

    // 計算碰撞點的速度
    glm::vec3 v_PA = v_cmA + glm::cross(angularVelocity, rA);
    glm::vec3 v_PB = v_cmB + glm::cross(other.angularVelocity, rB);

    // 計算相對速度
    glm::vec3 v_rel = v_PA - v_PB;

    // 沿法線方向的接近速度
    float v_n_close = glm::dot(v_rel, normal);

    // 如果物體正在分離，則不處理（這裡已經在 physicManager 中檢查）
    if (v_n_close >= 0) return;

    // 計算衝量大小 j
    float e = (restitution + other.restitution) * 0.5f; // 平均恢復係數
    float invMassA = (mass > 0.0f) ? 1.0f / mass : 0.0f;
    float invMassB = (other.mass > 0.0f) ? 1.0f / other.mass : 0.0f;

    // 計算逆慣性張量
    glm::mat3 invIA = glm::determinant(getWorldInertiaTensor()) != 0.0f ? glm::inverse(getWorldInertiaTensor()) : glm::mat3(0.0f);
    glm::mat3 invIB = glm::determinant(other.getWorldInertiaTensor()) != 0.0f ? glm::inverse(other.getWorldInertiaTensor()) : glm::mat3(0.0f);

    // 計算分母中的旋轉部分
    glm::vec3 rA_cross_n = glm::cross(rA, normal);
    glm::vec3 rB_cross_n = glm::cross(rB, normal);
    glm::vec3 invIA_term = glm::cross(invIA * rA_cross_n, rA);
    glm::vec3 invIB_term = glm::cross(invIB * rB_cross_n, rB);
    float denominator = invMassA + invMassB + glm::dot(normal, invIA_term) + glm::dot(normal, invIB_term);

    // 避免除以 0
    if (denominator <= 0.00001f) return;

    // 計算衝量大小
    float j = -(1.0f + e) * v_n_close / denominator;

    // 計算衝量向量
    glm::vec3 J = j * normal;

    // 更新質心線速度
    v_cmA += J * invMassA;
    v_cmB -= J * invMassB;

    // 更新角速度
    angularVelocity += invIA * glm::cross(rA, J);
    other.angularVelocity -= invIB * glm::cross(rB, J);

    // 更新物體速度（假設 velocity 是質心速度）
    velocity = v_cmA;
    other.velocity = v_cmB;

    // 簡單的穿透解決（可選）
    float penetration = 0.01f; // 需要更精確的計算
    position += normal * penetration * 0.5f;
    other.position -= normal * penetration * 0.5f;
}

float Object::GetSphereRadius() const {
    if (isSphere) {
        return sphereRadius;
    }
    return 0.0f; // Or return sphereWorld.radius, but that's the enclosing sphere for boxes
}


void Object::SetPosition(const glm::vec3& pos) {
    position = pos;
    updateOBB();
    updateAABB();
    updateBoundingSphere();
}

void Object::SetRotation(const glm::quat& rot) {
    rotation = glm::normalize(rot);
    updateOBB();
    updateAABB();
    updateBoundingSphere();
}

void Object::applyImpulse(const glm::vec3& impulse, const glm::vec3& applyPointWorld) {
    if (mass <= 0.0f) { // Cannot apply impulse to static or massless objects
        return;
    }

    // Apply linear impulse: Δv = J / m
    velocity += impulse / mass;

    // Apply angular impulse: Δω = I_world_inv * (r × J)
    // r is the vector from the center of mass to the point of application of the impulse
    glm::vec3 r = applyPointWorld - getWorldCenterOfMass();
    glm::vec3 angularImpulseMoment = glm::cross(r, impulse); // This is the torque τ_impulse part
    
    glm::mat3 worldInertiaTensor = getWorldInertiaTensor(); // Assumes this gets the non-inverse world inertia tensor
    glm::mat3 invWorldInertiaTensor;

    if (glm::determinant(worldInertiaTensor) != 0.0f) {
        invWorldInertiaTensor = glm::inverse(worldInertiaTensor);
    } else {
        // For objects with no rotational inertia (or mass 0, though caught above), 
        // inverse tensor is zero, so no change in angular velocity.
        invWorldInertiaTensor = glm::mat3(0.0f); 
    }

    angularVelocity += invWorldInertiaTensor * angularImpulseMoment;
}
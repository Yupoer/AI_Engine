#include "physicManager.h"

static glm::vec3 NormalizeVec3(const glm::vec3& v) {
    float lenSq = glm::dot(v, v);
    if (lenSq > 0.00001f) {
        return v / std::sqrt(lenSq);
    }
    return glm::vec3(0.0f);
}


PhysicManager::PhysicManager(float g) : gravityAcceleration(g), angularDragCoefficient(1.1f) {}

void PhysicManager::applyGravity(Object& obj) const {
    if (obj.mass > 0.0f) {
        obj.applyForce(glm::vec3(0.0f, -obj.mass * gravityAcceleration, 0.0f), obj.getWorldCenterOfMass());
    }
}

void PhysicManager::update(std::vector<Object*>& objects, const AABB& roomAABB, float deltaTime) {
    if (pausePhysics) return;

    for (Object* objPtr : objects) {
        if (!objPtr) continue;
        Object& obj = *objPtr;

        applyGravity(obj);
        obj.updatePhysics(deltaTime); // Object's own damping (linear and angular) is applied here

        // Apply global angular drag from PhysicManager
        if (glm::length(obj.angularVelocity) > 0.0001f && angularDragCoefficient > 0.0f) {
            // angularDragCoefficient of 0 means no drag, 1.0 means 100% drag per second (approximately)
            // Using std::pow for frame-rate independent damping:
            // (1.0f - coefficient) is the remaining part after one second.
            // Raises to power of deltaTime to get the effect for this frame.
            float dragFactor = glm::clamp(angularDragCoefficient, 0.0f, 1.0f); // Clamp to sensible range
            obj.angularVelocity *= std::pow(1.0f - dragFactor, deltaTime);
        }

        // --- Collision Detection and Response ---

        bool hasCollidedWithRoom = false; // 標記是否與房間發生碰撞
        glm::vec3 collisionNormal(0.0f);  // 將 collisionNormal 的宣告提前到此處

        // 1. Object vs Room (AABB)
        if (obj.IsSphereObject()) {
            // 球體 vs 房間
            BoundingSphere sphere = obj.GetBoundingSphere();
            if (AABB::SphereToAABB(sphere, roomAABB)) { // This is true if sphere intersects or is contained within roomAABB
                hasCollidedWithRoom = true;
                glm::vec3 sphereCenter = sphere.center;
                glm::vec3 closestPointInRoom = roomAABB.ClosestPoint(sphereCenter);

                glm::vec3 normalFromClosestPointToCenter = sphereCenter - closestPointInRoom;
                float distSqToClosestPoint = glm::dot(normalFromClosestPointToCenter, normalFromClosestPointToCenter);

                // Default collisionNormal (points from AABB surface towards sphere center if outside, or can be a fallback)
                // This will be refined by the 6-plane check later for impulse direction if needed.
                // For initial positional correction, the logic is more direct.

                if (distSqToClosestPoint > 0.000001f) {
                    // Sphere center is effectively OUTSIDE the closestPointInRoom (i.e., outside the AABB volume)
                    collisionNormal = glm::normalize(normalFromClosestPointToCenter); // Points from AABB to sphere center
                    float distanceToSurface = glm::sqrt(distSqToClosestPoint);
                    float penetrationDepth = sphere.radius - distanceToSurface;

                    if (penetrationDepth > 0.0f) {
                        // Apply initial positional correction to move sphere out of penetration
                        // We push along the direction from the AABB towards the sphere's center.
                        obj.SetPosition(sphereCenter + collisionNormal * (penetrationDepth + 0.001f));
                        sphere = obj.GetBoundingSphere(); // Update sphere state after position change
                    }
                } else {
                    // Sphere center is ON or INSIDE the AABB volume (closestPointInRoom is sphereCenter itself).
                    // No initial aggressive push based on radius. Rely on the 6-face definitive correction later.
                    // However, we still need a valid collisionNormal for impulse calc if v_n_close is negative.
                    // The 6-face check will give a better normal. For now, if we needed one here, it would be a fallback.
                    // This case implies the sphere is starting inside or has been pushed inside.
                    // The previous fallback normal logic was problematic here.
                    // We will let the 6-plane check determine the dominant collision normal if an impulse is needed.
                    // For now, initialize collisionNormal to something, but it might be overridden.
                    collisionNormal = glm::vec3(0,1,0); // A temporary placeholder, may not be used if no impulse.
                }

                // Dynamic impulse response
                // The contact point for impulse should be on the sphere surface facing the AABB collision point.
                // The 'collisionNormal' for impulse should ideally be from the 6-plane check if that's more specific.
                // However, the 6-plane check is AFTER impulse. This is tricky.
                // Let's use a 'currentCollisionNormalForImpulse' derived from the most penetrated face IF the 6-plane check logic was here.
                // For now, the 'collisionNormal' from above (if outside) or a fallback (if inside) will be used.
                // The previous logic used `closestPointInRoom - obj.getWorldCenterOfMass()` for `r_contact`,
                // and `sphere.center - collisionNormal * sphere.radius` for `contact_point_on_sphere_surface`.
                // Let's stick to using the more robust surface point for r_contact for now.

                glm::vec3 v_cm_before_impulse = obj.GetVelocity();
                glm::vec3 ang_vel_before_impulse = obj.angularVelocity;

                // Determine a reliable normal for IMPULSE. This is crucial.
                // The 6-plane check (now called definitive correction) finds the most relevant normal AFTER impulse.
                // For an impulse, we need it BEFORE.
                // If distSqToClosestPoint was > epsilon, 'collisionNormal' (AABB to sphere) is good.
                // If sphere center was inside, 'collisionNormal' is just a placeholder.
                // Let's try to get a better normal if inside by checking which face is *closest* to being penetrated or *is* penetrated.
                glm::vec3 impulseCollisionNormal = collisionNormal; // Start with the one we have.
                if (distSqToClosestPoint <= 0.000001f) { // If sphere center is inside/on AABB
                    // Find the face of AABB that sphere center is closest to crossing outwards or is crossing
                    float minDistToFace = FLT_MAX;
                    glm::vec3 tempNormal(0);
                    // Check -X face
                    float dist = sphere.center.x - roomAABB.GetMin().x;
                    if (dist < minDistToFace) { minDistToFace = dist; tempNormal = glm::vec3(1,0,0);}
                    // Check +X face
                    dist = roomAABB.GetMax().x - sphere.center.x;
                    if (dist < minDistToFace) { minDistToFace = dist; tempNormal = glm::vec3(-1,0,0);}
                    // Check -Y face
                    dist = sphere.center.y - roomAABB.GetMin().y;
                    if (dist < minDistToFace) { minDistToFace = dist; tempNormal = glm::vec3(0,1,0);}
                    // Check +Y face
                    dist = roomAABB.GetMax().y - sphere.center.y;
                    if (dist < minDistToFace) { minDistToFace = dist; tempNormal = glm::vec3(0,-1,0);}
                    // Check -Z face
                    dist = sphere.center.z - roomAABB.GetMin().z;
                    if (dist < minDistToFace) { minDistToFace = dist; tempNormal = glm::vec3(0,0,1);}
                    // Check +Z face
                    dist = roomAABB.GetMax().z - sphere.center.z;
                    if (dist < minDistToFace) { minDistToFace = dist; tempNormal = glm::vec3(0,0,-1);}
                    impulseCollisionNormal = tempNormal; // This normal points OUT from the AABB face
                }
                // Ensure impulseCollisionNormal is valid if it came from NormalizeVec3(0,0,0) earlier and wasn't updated
                 if (glm::dot(impulseCollisionNormal, impulseCollisionNormal) < 0.0001f) impulseCollisionNormal = glm::vec3(0,1,0); // Use dot product and adjust threshold for squared length


                glm::vec3 contact_point_on_sphere_surface = sphere.center - impulseCollisionNormal * sphere.radius;
                glm::vec3 r_contact = contact_point_on_sphere_surface - obj.getWorldCenterOfMass();
                
                glm::vec3 v_point = v_cm_before_impulse + glm::cross(ang_vel_before_impulse, r_contact);
                float v_n_close = glm::dot(v_point, impulseCollisionNormal); 

                float j_scalar_for_friction = 0.0f;

                // Check for actual contact with the plane defined by impulseCollisionNormal before applying impulse
                float distanceToCollisionPlane = FLT_MAX;
                float contact_epsilon = 0.05f; // Increased epsilon slightly for more robust contact detection

                if (glm::abs(impulseCollisionNormal.y) > 0.9f) { 
                    if (impulseCollisionNormal.y > 0.5f) { // Floor normal (0,1,0)
                        distanceToCollisionPlane = sphere.center.y - roomAABB.GetMin().y;
                    } else { // Ceiling normal (0,-1,0)
                        distanceToCollisionPlane = roomAABB.GetMax().y - sphere.center.y;
                    }
                } else if (glm::abs(impulseCollisionNormal.x) > 0.9f) { 
                    if (impulseCollisionNormal.x > 0.5f) { // Left wall of room, normal (1,0,0) from room to sphere
                        distanceToCollisionPlane = sphere.center.x - roomAABB.GetMin().x;
                    } else { // Right wall of room, normal (-1,0,0)
                        distanceToCollisionPlane = roomAABB.GetMax().x - sphere.center.x;
                    }
                } else if (glm::abs(impulseCollisionNormal.z) > 0.9f) { 
                    if (impulseCollisionNormal.z > 0.5f) { // Back wall of room, normal (0,0,1)
                        distanceToCollisionPlane = sphere.center.z - roomAABB.GetMin().z;
                    } else { // Front wall of room, normal (0,0,-1)
                        distanceToCollisionPlane = roomAABB.GetMax().z - sphere.center.z;
                    }
                }
                
                bool isTouchingSurfaceForImpulse = (distanceToCollisionPlane <= sphere.radius + contact_epsilon);

                if (isTouchingSurfaceForImpulse && v_n_close < 0.0f) { 
                    float e = obj.restitution;
                    float invMass = (obj.mass > 0.0f) ? 1.0f / obj.mass : 0.0f;
                    glm::mat3 invIA_world = obj.getWorldInertiaTensor();
                    if (glm::determinant(invIA_world) != 0.0f) invIA_world = glm::inverse(invIA_world);
                    else invIA_world = glm::mat3(0.0f);
                    
                    glm::vec3 r_cross_n = glm::cross(r_contact, impulseCollisionNormal);
                    glm::vec3 term_in_denominator = glm::cross(invIA_world * r_cross_n, r_contact);
                    float denominator = invMass + glm::dot(impulseCollisionNormal, term_in_denominator);
                    
                    if (denominator > 0.00001f) {
                        j_scalar_for_friction = -(1.0f + e) * v_n_close / denominator;
                    }
                    glm::vec3 J_impulse = j_scalar_for_friction * impulseCollisionNormal;
                    obj.SetVelocity(v_cm_before_impulse + J_impulse * invMass);
                    obj.angularVelocity = ang_vel_before_impulse + invIA_world * glm::cross(r_contact, J_impulse);

                    // --- Friction for sphere (applied after normal impulse) ---
                    glm::vec3 current_v_cm_after_normal_impulse = obj.GetVelocity(); 
                    glm::vec3 current_ang_vel_after_normal_impulse = obj.angularVelocity; 
                    glm::vec3 current_v_point_after_normal_impulse = current_v_cm_after_normal_impulse + glm::cross(current_ang_vel_after_normal_impulse, r_contact);
                    glm::vec3 tangent_vel = current_v_point_after_normal_impulse - glm::dot(current_v_point_after_normal_impulse, impulseCollisionNormal) * impulseCollisionNormal;
                    float tangent_speed = glm::length(tangent_vel);

                    if (tangent_speed > 0.0001f && invMass > 0.0f) { // ensure invMass is positive for friction
                        glm::vec3 tangent_dir = tangent_vel / tangent_speed;
                        glm::vec3 r_cross_t = glm::cross(r_contact, tangent_dir);
                        glm::vec3 term_friction_den_vec = glm::cross(invIA_world * r_cross_t, r_contact);
                        // Ensure invIA_world is not all zeros if using it in term_friction_den_vec
                        if (glm::determinant(invIA_world) == 0.0f && obj.mass > 0) { // if inertia tensor is zero, effectively point mass for rotation
                             term_friction_den_vec = glm::vec3(0.0f); // No rotational resistance term
                        }
                        float den_friction = invMass + glm::dot(tangent_dir, term_friction_den_vec);
                        
                        float jt_scalar = 0.0f;
                        if (std::abs(den_friction) > 0.00001f) {
                            jt_scalar = -tangent_speed / den_friction; 
                        }

                        float max_friction_impulse_mag = std::abs(j_scalar_for_friction) * obj.frictionCoeff;
                        if (std::abs(jt_scalar) > max_friction_impulse_mag) {
                            jt_scalar = glm::sign(jt_scalar) * max_friction_impulse_mag;
                        }
                        
                        glm::vec3 Jt_friction_impulse = jt_scalar * tangent_dir;
                        obj.SetVelocity(current_v_cm_after_normal_impulse + Jt_friction_impulse * invMass);
                        obj.angularVelocity = current_ang_vel_after_normal_impulse + invIA_world * glm::cross(r_contact, Jt_friction_impulse);
                    }
                } // End of dynamic impulse block (if v_n_close < 0)

                // --- POST-COLLISION DEFINITIVE CORRECTION FOR SPHERE (Re-check and ensure it's inside) ---
                // This runs if AABB::SphereToAABB was true, to enforce boundary conditions after any impulses.
                BoundingSphere finalSphereState = obj.GetBoundingSphere(); // Get current state
                glm::vec3 finalVel = obj.GetVelocity();

                // Floor
                if (finalSphereState.center.y - finalSphereState.radius < roomAABB.GetMin().y) {
                    float current_penetration_y = roomAABB.GetMin().y - (finalSphereState.center.y - finalSphereState.radius);
                    obj.SetPosition(obj.GetPosition() + glm::vec3(0.0f, current_penetration_y + 0.001f, 0.0f));
                    finalVel = obj.GetVelocity(); // Re-get velocity in case SetPosition changed something (it shouldn't)
                    if (finalVel.y < 0.0f) obj.SetVelocity(glm::vec3(finalVel.x, finalVel.y * -obj.restitution, finalVel.z));
                }
                // Ceiling
                if (finalSphereState.center.y + finalSphereState.radius > roomAABB.GetMax().y) {
                    float current_penetration_y = (finalSphereState.center.y + finalSphereState.radius) - roomAABB.GetMax().y;
                    obj.SetPosition(obj.GetPosition() - glm::vec3(0.0f, current_penetration_y + 0.001f, 0.0f));
                    finalVel = obj.GetVelocity();
                    if (finalVel.y > 0.0f) obj.SetVelocity(glm::vec3(finalVel.x, finalVel.y * -obj.restitution, finalVel.z));
                }
                // Left wall (-X)
                if (finalSphereState.center.x - finalSphereState.radius < roomAABB.GetMin().x) {
                    float current_penetration_x = roomAABB.GetMin().x - (finalSphereState.center.x - finalSphereState.radius);
                    obj.SetPosition(obj.GetPosition() + glm::vec3(current_penetration_x + 0.001f, 0.0f, 0.0f));
                    finalVel = obj.GetVelocity();
                    if (finalVel.x < 0.0f) obj.SetVelocity(glm::vec3(finalVel.x * -obj.restitution, finalVel.y, finalVel.z));
                }
                // Right wall (+X)
                if (finalSphereState.center.x + finalSphereState.radius > roomAABB.GetMax().x) {
                    float current_penetration_x = (finalSphereState.center.x + finalSphereState.radius) - roomAABB.GetMax().x;
                    obj.SetPosition(obj.GetPosition() - glm::vec3(current_penetration_x + 0.001f, 0.0f, 0.0f));
                    finalVel = obj.GetVelocity();
                    if (finalVel.x > 0.0f) obj.SetVelocity(glm::vec3(finalVel.x * -obj.restitution, finalVel.y, finalVel.z));
                }
                // Back wall (-Z)
                if (finalSphereState.center.z - finalSphereState.radius < roomAABB.GetMin().z) {
                    float current_penetration_z = roomAABB.GetMin().z - (finalSphereState.center.z - finalSphereState.radius);
                    obj.SetPosition(obj.GetPosition() + glm::vec3(0.0f, 0.0f, current_penetration_z + 0.001f));
                    finalVel = obj.GetVelocity();
                    if (finalVel.z < 0.0f) obj.SetVelocity(glm::vec3(finalVel.x, finalVel.y, finalVel.z * -obj.restitution));
                }
                // Front wall (+Z)
                if (finalSphereState.center.z + finalSphereState.radius > roomAABB.GetMax().z) {
                    float current_penetration_z = (finalSphereState.center.z + finalSphereState.radius) - roomAABB.GetMax().z;
                    obj.SetPosition(obj.GetPosition() - glm::vec3(0.0f, 0.0f, current_penetration_z + 0.001f));
                    finalVel = obj.GetVelocity();
                    if (finalVel.z > 0.0f) obj.SetVelocity(glm::vec3(finalVel.x, finalVel.y, finalVel.z * -obj.restitution));
                }
            }
        } else {
            // 非球體 vs 房間
            AABB worldObjAABB = obj.GetAABB();
            if (worldObjAABB.Intersects(roomAABB)) { // 先做粗略檢測
                hasCollidedWithRoom = true;
                float minPenetration = FLT_MAX;
                bool collisionOccurred = false;

                // X軸
                float d1 = worldObjAABB.GetMax().x - roomAABB.GetMin().x;
                float d2 = roomAABB.GetMax().x - worldObjAABB.GetMin().x;
                if (d1 < d2) { if (d1 < minPenetration) { minPenetration = d1; collisionNormal = glm::vec3(-1.0f, 0.0f, 0.0f); collisionOccurred = true; }}
                else { if (d2 < minPenetration) { minPenetration = d2; collisionNormal = glm::vec3(1.0f, 0.0f, 0.0f); collisionOccurred = true; }}
                // Y軸
                d1 = worldObjAABB.GetMax().y - roomAABB.GetMin().y;
                d2 = roomAABB.GetMax().y - worldObjAABB.GetMin().y;
                if (d1 < d2) { if (d1 < minPenetration) { minPenetration = d1; collisionNormal = glm::vec3(0.0f, -1.0f, 0.0f); collisionOccurred = true; }}
                else { if (d2 < minPenetration) { minPenetration = d2; collisionNormal = glm::vec3(0.0f, 1.0f, 0.0f); collisionOccurred = true; }}
                // Z軸
                d1 = worldObjAABB.GetMax().z - roomAABB.GetMin().z;
                d2 = roomAABB.GetMax().z - worldObjAABB.GetMin().z;
                if (d1 < d2) { if (d1 < minPenetration) { minPenetration = d1; collisionNormal = glm::vec3(0.0f, 0.0f, -1.0f); collisionOccurred = true; }}
                else { if (d2 < minPenetration) { minPenetration = d2; collisionNormal = glm::vec3(0.0f, 0.0f, 1.0f); collisionOccurred = true; }}
                
                // 修正的穿透檢測 (物體中心到牆面距離 vs 物體半徑)
                // 我們需要的是物體邊緣超出了房間邊界的量
                minPenetration = FLT_MAX; // 重置
                collisionOccurred = false;

                // 檢查各個面的穿透
                // 左牆 (-X)
                float currentPen = roomAABB.GetMin().x - worldObjAABB.GetMin().x;
                if (currentPen > 0 && currentPen < minPenetration) { minPenetration = currentPen; collisionNormal = glm::vec3(1.0f, 0.0f, 0.0f); collisionOccurred = true; }
                // 右牆 (+X)
                currentPen = worldObjAABB.GetMax().x - roomAABB.GetMax().x;
                if (currentPen > 0 && currentPen < minPenetration) { minPenetration = currentPen; collisionNormal = glm::vec3(-1.0f, 0.0f, 0.0f); collisionOccurred = true; }
                // 地板 (-Y)
                currentPen = roomAABB.GetMin().y - worldObjAABB.GetMin().y;
                if (currentPen > 0 && currentPen < minPenetration) { minPenetration = currentPen; collisionNormal = glm::vec3(0.0f, 1.0f, 0.0f); collisionOccurred = true; }
                // 天花板 (+Y)
                currentPen = worldObjAABB.GetMax().y - roomAABB.GetMax().y;
                if (currentPen > 0 && currentPen < minPenetration) { minPenetration = currentPen; collisionNormal = glm::vec3(0.0f, -1.0f, 0.0f); collisionOccurred = true; }
                // 後牆 (-Z)
                currentPen = roomAABB.GetMin().z - worldObjAABB.GetMin().z;
                if (currentPen > 0 && currentPen < minPenetration) { minPenetration = currentPen; collisionNormal = glm::vec3(0.0f, 0.0f, 1.0f); collisionOccurred = true; }
                // 前牆 (+Z)
                currentPen = worldObjAABB.GetMax().z - roomAABB.GetMax().z;
                if (currentPen > 0 && currentPen < minPenetration) { minPenetration = currentPen; collisionNormal = glm::vec3(0.0f, 0.0f, -1.0f); collisionOccurred = true; }


                if (collisionOccurred && minPenetration < (worldObjAABB.GetMax().x - worldObjAABB.GetMin().x) ) { // 確保穿透小於物體尺寸 (避免完全穿過)
                                                                                                            // 並且 minPenetration > 0
                    if (minPenetration < 0.0001f && minPenetration != FLT_MAX) minPenetration = 0.0001f; // 避免過小推移

                    obj.SetPosition(obj.GetPosition() + collisionNormal * (minPenetration + 0.001f)); // 位置校正，稍微多推一點

                    // Special handling for ceiling for AABB too
                    if (collisionNormal.y < -0.9f && obj.GetVelocity().y > 0.0f) { 
                        glm::vec3 vel = obj.GetVelocity();
                        obj.SetVelocity(glm::vec3(vel.x, vel.y * -obj.restitution * 0.5f, vel.z));
                    }

                    glm::vec3 objHalfExtents = (worldObjAABB.GetMax() - worldObjAABB.GetMin()) * 0.5f;
                    glm::vec3 r_local(0.0f); // 從AABB中心到碰撞面的向量（局部坐標）
                    if (std::abs(collisionNormal.x) > 0.5f) r_local.x = (collisionNormal.x > 0) ? -objHalfExtents.x : objHalfExtents.x; // 碰撞法線指向外，所以r_local指向內
                    else if (std::abs(collisionNormal.y) > 0.5f) r_local.y = (collisionNormal.y > 0) ? -objHalfExtents.y : objHalfExtents.y;
                    else if (std::abs(collisionNormal.z) > 0.5f) r_local.z = (collisionNormal.z > 0) ? -objHalfExtents.z : objHalfExtents.z;
                    
                    glm::vec3 r_contact = obj.GetRotation() * r_local; // 將r_local旋轉到世界坐標系 (這是從質心到碰撞點的向量)

                    glm::vec3 v_cm = obj.GetVelocity();
                    glm::vec3 v_point = v_cm + glm::cross(obj.angularVelocity, r_contact);
                    float v_n_close = glm::dot(v_point, collisionNormal);

                    float j_scalar = 0.0f;
                    if (v_n_close < 0.0f) { // 法向反彈
                        float e = obj.restitution;
                        float invMass = (obj.mass > 0.0f) ? 1.0f / obj.mass : 0.0f;
                        glm::mat3 invIA_world = obj.getWorldInertiaTensor();
                        if (glm::determinant(invIA_world) != 0.0f) invIA_world = glm::inverse(invIA_world);
                        else invIA_world = glm::mat3(0.0f);

                        glm::vec3 r_cross_n = glm::cross(r_contact, collisionNormal);
                        glm::vec3 term_in_denominator = glm::cross(invIA_world * r_cross_n, r_contact);
                        float denominator = invMass + glm::dot(collisionNormal, term_in_denominator);
                        
                        if (denominator > 0.00001f) {
                            j_scalar = -(1.0f + e) * v_n_close / denominator;
                        }
                        glm::vec3 J_impulse = j_scalar * collisionNormal;
                        obj.SetVelocity(v_cm + J_impulse * invMass);
                        obj.angularVelocity += invIA_world * glm::cross(r_contact, J_impulse);
                    }

                    // 滑動摩擦 (非球體 vs 房間)
                    glm::vec3 current_v_cm = obj.GetVelocity(); // 使用更新後的速度
                    glm::vec3 current_ang_vel = obj.angularVelocity;
                    glm::vec3 current_v_point = current_v_cm + glm::cross(current_ang_vel, r_contact);
                    glm::vec3 tangent_vel = current_v_point - glm::dot(current_v_point, collisionNormal) * collisionNormal;
                    float tangent_speed = glm::length(tangent_vel);

                    if (tangent_speed > 0.0001f) {
                        glm::vec3 tangent_dir = tangent_vel / tangent_speed;
                        float invMass = (obj.mass > 0.0f) ? 1.0f / obj.mass : 0.0f;
                        glm::mat3 invIA_world = obj.getWorldInertiaTensor();
                        if (glm::determinant(invIA_world) != 0.0f) invIA_world = glm::inverse(invIA_world); else invIA_world = glm::mat3(0.0f);

                        glm::vec3 r_cross_t = glm::cross(r_contact, tangent_dir);
                        glm::vec3 term_friction_den = glm::cross(invIA_world * r_cross_t, r_contact);
                        float den_friction = invMass + glm::dot(tangent_dir, term_friction_den);

                        float jt_scalar = 0.0f;
                        if (std::abs(den_friction) > 0.00001f) {
                             jt_scalar = -tangent_speed / den_friction;
                        }
                        
                        float max_friction_impulse_mag;
                        if (j_scalar != 0.0f) { // 如果有法向碰撞衝量 (j_scalar是法向衝量的大小)
                            max_friction_impulse_mag = std::abs(j_scalar * obj.frictionCoeff);
                        } else { // 靜止或僅滑動接觸，沒有顯著法向碰撞
                            float normal_force_approx = obj.mass * gravityAcceleration; // 近似法向力
                            max_friction_impulse_mag = normal_force_approx * obj.frictionCoeff * deltaTime; // 更像是 力*dt
                        }
                        
                        if (std::abs(jt_scalar) > max_friction_impulse_mag && max_friction_impulse_mag > 0.0f) {
                            jt_scalar = glm::sign(jt_scalar) * max_friction_impulse_mag;
                        } else if (max_friction_impulse_mag <= 0.0f) { // 如果沒有法向力/壓力，則沒有摩擦
                            jt_scalar = 0.0f;
                        }

                        glm::vec3 Jt_friction_impulse = jt_scalar * tangent_dir;
                        obj.SetVelocity(obj.GetVelocity() + Jt_friction_impulse * invMass);
                        obj.angularVelocity += invIA_world * glm::cross(r_contact, Jt_friction_impulse);
                    }

                    // 簡化的滾動摩擦 (只在接觸地板時，且有角速度)
                    if (std::abs(collisionNormal.y - 1.0f) < 0.1f && glm::length(obj.angularVelocity) > 0.01f) {
                        obj.angularVelocity *= (1.0f - 0.7f * deltaTime); // Increased damping factor to 0.7f
                        if (glm::length(obj.angularVelocity) < 0.03f) { 
                            obj.angularVelocity = glm::vec3(0.0f);
                        }
                    }

                    // --- POST-COLLISION DEFINITIVE CORRECTION (AABB) - from custom_tool_13 ---
                    AABB currentAABBState = obj.GetAABB(); 
                    glm::vec3 currentVelAABB = obj.GetVelocity();
                    bool stillPenetratingAABB = false;
                    glm::vec3 finalCollisionNormalAABB = collisionNormal; // Initial collisionNormal from AABB detection

                    // Floor for AABB
                    if (currentAABBState.GetMin().y < roomAABB.GetMin().y) {
                        float penY = roomAABB.GetMin().y - currentAABBState.GetMin().y;
                        obj.SetPosition(obj.GetPosition() + glm::vec3(0.0f, penY + 0.0001f, 0.0f));
                        if (currentVelAABB.y < 0.0f) obj.SetVelocity(glm::vec3(currentVelAABB.x, currentVelAABB.y * -obj.restitution * 0.8f, currentVelAABB.z));
                        stillPenetratingAABB = true; finalCollisionNormalAABB = glm::vec3(0,1,0);
                    }
                    // Ceiling for AABB
                    if (currentAABBState.GetMax().y > roomAABB.GetMax().y) {
                        float penY = currentAABBState.GetMax().y - roomAABB.GetMax().y;
                        obj.SetPosition(obj.GetPosition() - glm::vec3(0.0f, penY + 0.0001f, 0.0f));
                        if (currentVelAABB.y > 0.0f) obj.SetVelocity(glm::vec3(currentVelAABB.x, currentVelAABB.y * -obj.restitution * 0.8f, currentVelAABB.z));
                        stillPenetratingAABB = true; finalCollisionNormalAABB = glm::vec3(0,-1,0);
                    }
                    // Left Wall (-X) for AABB
                    if (currentAABBState.GetMin().x < roomAABB.GetMin().x) {
                        float penX = roomAABB.GetMin().x - currentAABBState.GetMin().x;
                        obj.SetPosition(obj.GetPosition() + glm::vec3(penX + 0.0001f, 0.0f, 0.0f));
                        if (currentVelAABB.x < 0.0f) obj.SetVelocity(glm::vec3(currentVelAABB.x * -obj.restitution * 0.8f, currentVelAABB.y, currentVelAABB.z));
                        stillPenetratingAABB = true; finalCollisionNormalAABB = glm::vec3(1,0,0);
                    }
                    // Right Wall (+X) for AABB
                    if (currentAABBState.GetMax().x > roomAABB.GetMax().x) {
                        float penX = currentAABBState.GetMax().x - roomAABB.GetMax().x;
                        obj.SetPosition(obj.GetPosition() - glm::vec3(penX + 0.0001f, 0.0f, 0.0f));
                        if (currentVelAABB.x > 0.0f) obj.SetVelocity(glm::vec3(currentVelAABB.x * -obj.restitution * 0.8f, currentVelAABB.y, currentVelAABB.z));
                        stillPenetratingAABB = true; finalCollisionNormalAABB = glm::vec3(-1,0,0);
                    }
                    // Back Wall (-Z) for AABB
                    if (currentAABBState.GetMin().z < roomAABB.GetMin().z) {
                        float penZ = roomAABB.GetMin().z - currentAABBState.GetMin().z;
                        obj.SetPosition(obj.GetPosition() + glm::vec3(0.0f, 0.0f, penZ + 0.0001f));
                        if (currentVelAABB.z < 0.0f) obj.SetVelocity(glm::vec3(currentVelAABB.x, currentVelAABB.y, currentVelAABB.z * -obj.restitution * 0.8f));
                        stillPenetratingAABB = true; finalCollisionNormalAABB = glm::vec3(0,0,1);
                    }
                    // Front Wall (+Z) for AABB
                    if (currentAABBState.GetMax().z > roomAABB.GetMax().z) {
                        float penZ = currentAABBState.GetMax().z - roomAABB.GetMax().z;
                        obj.SetPosition(obj.GetPosition() - glm::vec3(0.0f, 0.0f, penZ + 0.0001f));
                        if (currentVelAABB.z > 0.0f) obj.SetVelocity(glm::vec3(currentVelAABB.x, currentVelAABB.y, currentVelAABB.z * -obj.restitution * 0.8f));
                        stillPenetratingAABB = true; finalCollisionNormalAABB = glm::vec3(0,0,-1);
                    }

                    if (stillPenetratingAABB) {
                        glm::vec3 finalVelAABB = obj.GetVelocity();
                        float velAlongFinalNormalAABB = glm::dot(finalVelAABB, finalCollisionNormalAABB);
                        if (velAlongFinalNormalAABB > 0.0f) {
                            obj.SetVelocity(finalVelAABB - finalCollisionNormalAABB * velAlongFinalNormalAABB);
                        }
                    }
                }
            }
        }

        // 在所有碰撞處理之後，可以添加一個全局的靜止判斷 (可選)
        // 這個判斷可以基於物體是否與某個表面接觸，並且速度非常低
        // 例如，如果 obj.isOnSurface() && obj.isEffectivelyStill()
        // 此處的 hasCollidedWithRoom 和 collisionNormal 只反映了與房間的最新碰撞
        if (hasCollidedWithRoom && std::abs(collisionNormal.y - 1.0f) < 0.1f) { // 假設與地板接觸
            float linearSpeed = glm::length(obj.GetVelocity());
            float angularSpeed = glm::length(obj.angularVelocity); // 再次檢查角速度
            float linearRestThreshold = 0.05f;  // Increased threshold slightly
            float angularRestThreshold = 0.05f; // Increased threshold slightly

            // 檢查是否有顯著的外部持續力 (簡化判斷)
            // bool significantExternalForce = glm::length(obj.forceAccumulator) > (obj.mass * gravityAcceleration * 0.1f);
            // if (significantExternalForce) printf("Significant force: %f\n", glm::length(obj.forceAccumulator));

            if (linearSpeed < linearRestThreshold && angularSpeed < angularRestThreshold ) { // && !significantExternalForce) {
                 // 為了防止因重力反覆觸發，只有當合力也小的時候才完全停止
                 // 或者，如果只是在地板上，並且速度非常小，我們可以更積極地停止它
                 // printf("Object %p nearly at rest. Linear: %f, Angular: %f\n", &obj, linearSpeed, angularSpeed);
                 obj.SetVelocity(obj.GetVelocity() * 0.3f); // Stronger damping when near rest
                 obj.angularVelocity *= 0.3f;
                 if (linearSpeed < 0.01f) obj.SetVelocity(glm::vec3(0.0f));
                 if (angularSpeed < 0.01f) obj.angularVelocity = glm::vec3(0.0f);
            }
        }

        // 2. Object vs Object
        for (Object* otherPtr : objects) {
            if (!otherPtr || objPtr == otherPtr) continue;
            Object& other = *otherPtr;

            bool collisionDetectedThisPair = false;
            glm::vec3 normalForThisPair(0.0f);
            glm::vec3 pointOfContactThisPair(0.0f);

            // --- 碰撞檢測邏輯開始 ---
            if (obj.IsSphereObject() && other.IsSphereObject()) {
                // 球體 vs 球體
                if (AABB::SphereToSphere(obj.GetBoundingSphere(), other.GetBoundingSphere())) {
                    collisionDetectedThisPair = true;
                    normalForThisPair = NormalizeVec3(other.GetBoundingSphere().center - obj.GetBoundingSphere().center);
                    // 接觸點可以取兩個球心連線的中點，或者一個球體表面
                    pointOfContactThisPair = obj.GetBoundingSphere().center + normalForThisPair * obj.GetBoundingSphere().radius;
                }
            } else if (obj.IsSphereObject() && !other.IsSphereObject()) {
                // 球體 (obj) vs AABB (other)
                if (AABB::SphereToAABB(obj.GetBoundingSphere(), other.GetAABB())) {
                    collisionDetectedThisPair = true;
                    pointOfContactThisPair = other.GetAABB().ClosestPoint(obj.GetBoundingSphere().center);
                    normalForThisPair = NormalizeVec3(obj.GetBoundingSphere().center - pointOfContactThisPair);
                    // 可選：將接觸點精確移到球體表面
                    // pointOfContactThisPair = obj.GetBoundingSphere().center - normalForThisPair * obj.GetBoundingSphere().radius;
                }
            } else if (!obj.IsSphereObject() && other.IsSphereObject()) {
                // AABB (obj) vs 球體 (other)
                if (AABB::SphereToAABB(other.GetBoundingSphere(), obj.GetAABB())) {
                    collisionDetectedThisPair = true;
                    pointOfContactThisPair = obj.GetAABB().ClosestPoint(other.GetBoundingSphere().center);
                    normalForThisPair = NormalizeVec3(other.GetBoundingSphere().center - pointOfContactThisPair);
                }
            } else {
                // AABB (obj) vs AABB (other)
                if (obj.GetAABB().Intersects(other.GetAABB())) {
                    collisionDetectedThisPair = true;
                    // 簡化 AABB-AABB 的法線和接觸點計算
                    // 法線可以從一個中心指向另一個中心，或者更複雜的 SAT 分離軸
                    normalForThisPair = NormalizeVec3(other.GetPosition() - obj.GetPosition()); 
                    // 接觸點可以近似為兩個AABB中心的連線與其中一個AABB表面的交點
                    // 或者更簡單地，取兩個AABB重疊區域的中心（如果能計算出來的話）
                    // 這裡使用一個非常簡化的版本：兩個物體位置的中點
                    pointOfContactThisPair = (obj.GetPosition() + other.GetPosition()) * 0.5f;
                    // 注意：這個簡化的 AABB-AABB 接觸點和法線可能導致不夠真實的物理行為，
                    // 特別是對於旋轉。更精確的方法（如SAT）會更複雜。
                }
            }
            // --- 碰撞檢測邏輯結束 ---

            if (collisionDetectedThisPair) {
                // --- Penetration Resolution (簡化版) ---
                float penetrationDepth = 0.0f;
                glm::vec3 resolutionNormal = normalForThisPair; // Normal from obj to other
                bool needsResolution = false;

                if (obj.IsSphereObject() && other.IsSphereObject()) {
                    BoundingSphere s1 = obj.GetBoundingSphere();
                    BoundingSphere s2 = other.GetBoundingSphere();
                    float distCenters = glm::distance(s1.center, s2.center);
                    float sumRadii = s1.radius + s2.radius;
                    if (distCenters < sumRadii) {
                        penetrationDepth = sumRadii - distCenters;
                        // resolutionNormal is already other.center - obj.center, normalized.
                        // We need to move obj by -resolutionNormal * depth/2 and other by resolutionNormal * depth/2
                        needsResolution = true;
                    }
                } else {
                    // Simplified AABB based penetration for Sphere-AABB and AABB-AABB
                    // This is a very rough approximation and doesn't handle rotation well for OBBs
                    AABB aabb1 = obj.GetAABB(); // World AABB
                    AABB aabb2 = other.GetAABB(); // World AABB
                    
                    // Check for overlap along axes (world axes for AABB)
                    glm::vec3 overlap(0.0f);
                    overlap.x = std::max(0.0f, std::min(aabb1.GetMax().x, aabb2.GetMax().x) - std::max(aabb1.GetMin().x, aabb2.GetMin().x));
                    overlap.y = std::max(0.0f, std::min(aabb1.GetMax().y, aabb2.GetMax().y) - std::max(aabb1.GetMin().y, aabb2.GetMin().y));
                    overlap.z = std::max(0.0f, std::min(aabb1.GetMax().z, aabb2.GetMax().z) - std::max(aabb1.GetMin().z, aabb2.GetMin().z));

                    if (overlap.x > 0.0f && overlap.y > 0.0f && overlap.z > 0.0f) {
                        // Find axis of minimum overlap for resolution normal
                        if (overlap.x < overlap.y && overlap.x < overlap.z) {
                            penetrationDepth = overlap.x;
                            resolutionNormal = (obj.GetPosition().x < other.GetPosition().x) ? glm::vec3(-1,0,0) : glm::vec3(1,0,0);
                        } else if (overlap.y < overlap.z) {
                            penetrationDepth = overlap.y;
                            resolutionNormal = (obj.GetPosition().y < other.GetPosition().y) ? glm::vec3(0,-1,0) : glm::vec3(0,1,0);
                        } else {
                            penetrationDepth = overlap.z;
                            resolutionNormal = (obj.GetPosition().z < other.GetPosition().z) ? glm::vec3(0,0,-1) : glm::vec3(0,0,1);
                        }
                        // resolutionNormal now points from other towards obj along axis of min penetration
                        // We want to move obj along resolutionNormal and other along -resolutionNormal
                        needsResolution = true;
                    }
                }

                if (needsResolution && penetrationDepth > 0.0001f) {
                    float totalMass = obj.mass + other.mass;
                    float invMassObj = (obj.mass > 0 && totalMass > 0) ? obj.mass / totalMass : 0.5f; // Avoid division by zero, default to equal split
                    float invMassOther = (other.mass > 0 && totalMass > 0) ? other.mass / totalMass : 0.5f;
                    if (obj.mass <=0) { invMassObj = 0.0f; invMassOther = 1.0f; } // If obj is static, other moves all
                    if (other.mass <=0) { invMassOther = 0.0f; invMassObj = 1.0f; } // If other is static, obj moves all
                    if (obj.mass <=0 && other.mass <=0) {invMassObj = 0.0f; invMassOther = 0.0f;} // both static, no move


                    // resolutionNormal for Sphere-Sphere was from obj to other.
                    // resolutionNormal for AABB-AABB was from other to obj.
                    // Let's be consistent: normal points from obj towards other.
                    // For Sphere-Sphere: normalForThisPair is other.center - obj.center (OK)
                    // For AABB-AABB: if we want normal from obj to other, it should be other.pos - obj.pos direction.
                    // The current AABB resolutionNormal calculation tries to point away from the overlap for obj.
                    // Let's use normalForThisPair (obj to other) for the direction of pushing 'other'.

                    // We want to push 'obj' by -normalForThisPair and 'other' by normalForThisPair
                    glm::vec3 correctionDirObj = -normalForThisPair; // normalForThisPair is from obj towards other
                    glm::vec3 correctionDirOther = normalForThisPair;

                    // For AABB vs AABB, the 'resolutionNormal' found from min overlap axis might be better than normalForThisPair (center to center).
                    // If not Sphere-Sphere, use the axis-aligned resolution normal.
                    if (!(obj.IsSphereObject() && other.IsSphereObject())) {
                         // The AABB resolutionNormal was calculated to push obj away from other.
                         // So obj moves along resolutionNormal, other moves by -resolutionNormal
                         correctionDirObj = resolutionNormal; 
                         correctionDirOther = -resolutionNormal;
                    }

                    // Apply positional correction
                    if (obj.mass > 0) {
                         obj.SetPosition(obj.GetPosition() + correctionDirObj * penetrationDepth * invMassOther); // obj moves by other's mass proportion
                    }
                    if (other.mass > 0) {
                         other.SetPosition(other.GetPosition() + correctionDirOther * penetrationDepth * invMassObj); // other moves by obj's mass proportion
                    }
                     // After resolution, update bounding volumes if necessary for subsequent impulse calculation
                    // obj.UpdateBoundingVolumes(); // Assuming such a method exists
                    // other.UpdateBoundingVolumes();
                }

                // 計算質心世界座標
                glm::vec3 cmA = obj.getWorldCenterOfMass();
                glm::vec3 cmB = other.getWorldCenterOfMass();

                // 計算 r_A 和 r_B（質心到碰撞點的向量）
                glm::vec3 rA = pointOfContactThisPair - cmA;
                glm::vec3 rB = pointOfContactThisPair - cmB;

                // 計算質心速度
                glm::vec3 v_cmA = obj.GetVelocity();
                glm::vec3 v_cmB = other.GetVelocity();

                // 計算碰撞點的速度
                glm::vec3 v_PA = v_cmA + glm::cross(obj.angularVelocity, rA);
                glm::vec3 v_PB = v_cmB + glm::cross(other.angularVelocity, rB);

                // 計算相對速度
                glm::vec3 v_rel = v_PA - v_PB;

                // 沿法線方向的接近速度 (使用 normalForThisPair)
                float v_n_close = glm::dot(v_rel, normalForThisPair);

                // 如果物體正在分離（或平行移動），則不處理碰撞
                if (v_n_close >= 0.0f) {
                    continue; // 跳過此碰撞對的響應
                }

                // 調用 Object 類的 handleCollision 方法來處理衝量和速度更新
                // normalForThisPair 是從 obj 指向 other 的方向
                obj.handleCollision(other, pointOfContactThisPair, normalForThisPair);
            }
        } // 結束對 otherPtr 的迴圈
    } // 結束對 objPtr 的迴圈
} // 結束 PhysicManager::update 方法
#include "DrawBall.h"
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <algorithm>

DrawBall::DrawBall(GLuint vao, int vc, float radius)
    : VAO(vao), vertexCount(vc),
      position(0.0f), velocity(0.0f), acceleration(0.0f),
      scale(radius), gravity(-9.8f),
      color(0.93f, 0.16f, 0.16f),
      isPredator(false),
      isStationary(false),
      score(0),
      point(0),
      currentState(FSMState::SelectTarget),
      targetPrey(nullptr),
      lastTargetSelectionTime(0.0f),
      predatorSpeed(5.0f) {
    this->boundingBox.radius = radius;
    this->boundingBox.center = position;
    UpdateBoundingSphere();
}

DrawBall::~DrawBall() {}

void DrawBall::UpdateBoundingSphere() {
    this->boundingBox.SetCenter(position);
}

void DrawBall::Update(float deltaTime, const AABB& roomAABB) {
    if (isStationary) {
        return;
    }
    velocity.y += gravity * deltaTime;
    position += velocity * deltaTime;
    glm::vec3 roomMin = roomAABB.GetMin();
    glm::vec3 roomMax = roomAABB.GetMax();
    if (position.y - scale < roomMin.y) {
        position.y = roomMin.y + scale;
        velocity.y *= -1.0f;
    }
    if (position.x - scale < roomMin.x || position.x + scale > roomMax.x) {
        velocity.x *= -1.0f;
        position.x = glm::clamp(position.x, roomMin.x + scale, roomMax.x - scale);
    }
    if (position.z - scale < roomMin.z || position.z + scale > roomMax.z) {
        velocity.z *= -1.0f;
        position.z = glm::clamp(position.z, roomMin.z + scale, roomMax.z - scale);
    }
    UpdateBoundingSphere();
} 

void DrawBall::Update(float deltaTime, const AABB& roomAABB, const std::vector<DrawBall*>& balls) {
    if (isStationary) {
        return;
    }
    if (isPredator) {
        glm::vec3 color = GetColor();
        bool isGrayPredator = (color.r > 0.4f && color.g > 0.4f && color.b > 0.4f);
        if (isGrayPredator) {
            UpdateFSM(deltaTime, balls);
        } else {
            UpdateFuzzyLogic(deltaTime, balls);
        }
    } else {
        glm::vec3 avoidanceForce(0.0f);
        float avoidanceRadius = 2.0f;
        for (const auto& predator : balls) {
            if (predator->IsPredator()) {
                glm::vec3 toPredator = predator->GetPosition() - position;
                float distance = glm::length(toPredator);
                if (distance < avoidanceRadius && distance > 0.001f) {
                    glm::vec3 avoidDirection = -glm::normalize(toPredator);
                    avoidDirection.y = 0.0f;
                    avoidDirection = glm::normalize(avoidDirection);
                    float avoidStrength = (avoidanceRadius - distance) / avoidanceRadius;
                    avoidanceForce += avoidDirection * avoidStrength * 3.0f;
                }
            }
        }
        float baseSpeed = 0.0f;
        if (point == 15) baseSpeed = 4.0f;
        else if (point == 10) baseSpeed = 3.0f;
        else if (point == 5) baseSpeed = 2.0f;
        if (glm::length(avoidanceForce) > 0.001f) {
            velocity.x = glm::clamp(velocity.x + avoidanceForce.x * deltaTime, -baseSpeed, baseSpeed);
            velocity.z = glm::clamp(velocity.z + avoidanceForce.z * deltaTime, -baseSpeed, baseSpeed);
        } else {
            velocity.x = glm::clamp(velocity.x, -baseSpeed, baseSpeed);
            velocity.z = glm::clamp(velocity.z, -baseSpeed, baseSpeed);
        }
    }
    velocity.y += gravity * deltaTime;
    position += velocity * deltaTime;
    glm::vec3 roomMin = roomAABB.GetMin();
    glm::vec3 roomMax = roomAABB.GetMax();
    if (position.y - scale < roomMin.y) {
        position.y = roomMin.y + scale;
        velocity.y *= -1.0f;
    }
    if (position.x - scale < roomMin.x || position.x + scale > roomMax.x) {
        velocity.x *= -1.0f;
        position.x = glm::clamp(position.x, roomMin.x + scale, roomMax.x - scale);
    }
    if (position.z - scale < roomMin.z || position.z + scale > roomMax.z) {
        velocity.z *= -1.0f;
        position.z = glm::clamp(position.z, roomMin.z + scale, roomMax.z - scale);
    }
    UpdateBoundingSphere();
} 

// FSM AI Engine for Gray Predator
void DrawBall::UpdateFSM(float deltaTime, const std::vector<DrawBall*>& preys) {
    lastTargetSelectionTime += deltaTime;
    
    switch (currentState) {
        case FSMState::SelectTarget: {
            // Select target every 0.5 seconds or if no target
            if (targetPrey == nullptr || lastTargetSelectionTime > 0.5f) {
                targetPrey = SelectTargetFSM(preys);
                lastTargetSelectionTime = 0.0f;
                
                if (targetPrey != nullptr) {
                    currentState = FSMState::ChaseTarget;
                }
            }
            break;
        }
        
        case FSMState::ChaseTarget: {
            if (targetPrey == nullptr) {
                // Target is gone (eaten by other predator), return to select
                currentState = FSMState::SelectTarget;
                velocity.x = 0.0f;
                velocity.z = 0.0f;
            } else {
                // Check if target still exists in prey list
                bool targetExists = false;
                for (auto prey : preys) {
                    if (prey == targetPrey) {
                        targetExists = true;
                        break;
                    }
                }
                
                if (!targetExists) {
                    // Target was eaten, return to select immediately
                    targetPrey = nullptr;
                    currentState = FSMState::SelectTarget;
                    lastTargetSelectionTime = 0.5f; // Force immediate selection
                    velocity.x = 0.0f;
                    velocity.z = 0.0f;
                } else {
                    // Check if target is too far (invalid)
                    float distance = glm::length(targetPrey->GetPosition() - position);
                    if (distance > 8.0f) {
                        // Target too far, select new target
                        targetPrey = nullptr;
                        currentState = FSMState::SelectTarget;
                        velocity.x = 0.0f;
                        velocity.z = 0.0f;
                    } else {
                        // Chase the target
                        ChaseTarget(deltaTime);
                    }
                }
            }
            break;
        }
    }
}

// FSM Target Selection: Choose highest value prey within shortest distance
DrawBall* DrawBall::SelectTargetFSM(const std::vector<DrawBall*>& preys) {
    DrawBall* bestTarget = nullptr;
    float bestScore = -1.0f;
    
    for (auto prey : preys) {
        if (prey->IsPredator()) continue; // Skip other predators
        
        float distance = glm::length(prey->GetPosition() - position);
        if (distance > 10.0f) continue; // Only consider nearby preys
        
        // FSM Logic: Prioritize high value prey with short distance
        // Score = Value / Distance (higher is better)
        float score = static_cast<float>(prey->GetPoint()) / (distance + 0.1f);
        
        if (score > bestScore) {
            bestScore = score;
            bestTarget = prey;
        }
    }
    
    return bestTarget;
}

// Fuzzy Logic AI Engine for Purple Predator
void DrawBall::UpdateFuzzyLogic(float deltaTime, const std::vector<DrawBall*>& preys) {
    lastTargetSelectionTime += deltaTime;
    
    // Select target every 1.0 seconds using fuzzy logic
    if (targetPrey == nullptr || lastTargetSelectionTime > 1.0f) {
        targetPrey = SelectTargetFuzzy(preys);
        lastTargetSelectionTime = 0.0f;
    }
    
    if (targetPrey != nullptr) {
        // Check if target still exists in prey list
        bool targetExists = false;
        for (auto prey : preys) {
            if (prey == targetPrey) {
                targetExists = true;
                break;
            }
        }
        
        if (!targetExists) {
            // Target was eaten, force immediate reselection
            targetPrey = nullptr;
            lastTargetSelectionTime = 1.0f; // Force immediate selection
            velocity.x = 0.0f;
            velocity.z = 0.0f;
        } else {
            // Check if target is still valid
            float distance = glm::length(targetPrey->GetPosition() - position);
            if (distance > 8.0f) {
                targetPrey = nullptr; // Target too far
                velocity.x = 0.0f;
                velocity.z = 0.0f;
            } else {
                ChaseTarget(deltaTime);
            }
        }
    } else {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
    }
}

// Fuzzy Logic Target Selection
DrawBall* DrawBall::SelectTargetFuzzy(const std::vector<DrawBall*>& preys) {
    DrawBall* bestTarget = nullptr;
    float bestPriority = 0.0f;
    
    for (auto prey : preys) {
        if (prey->IsPredator()) continue; // Skip other predators
        
        float distance = glm::length(prey->GetPosition() - position);
        if (distance > 7.0f) continue; // Only consider reachable preys
        
        FuzzyInput input;
        input.distance = distance;
        input.preyValue = prey->GetPoint();
        
        float priority = CalculateFuzzyPriority(input);
        
        if (priority > bestPriority) {
            bestPriority = priority;
            bestTarget = prey;
        }
    }
    
    return bestTarget;
}

// Chase Target Implementation
void DrawBall::ChaseTarget(float deltaTime) {
    if (targetPrey == nullptr) return;
    
    glm::vec3 direction = targetPrey->GetPosition() - position;
    direction.y = 0.0f; // Only move horizontally
    
    if (glm::length(direction) > 0.001f) {
        direction = glm::normalize(direction);
        velocity.x = direction.x * predatorSpeed;
        velocity.z = direction.z * predatorSpeed;
    }
}

// Fuzzy Logic Implementation
float DrawBall::CalculateFuzzyPriority(const FuzzyInput& input) {
    // Fuzzy Rules:
    // 1. If distance is Close and value is High, then priority is VeryHigh
    // 2. If distance is Close and value is Medium, then priority is High  
    // 3. If distance is Medium and value is High, then priority is High
    // 4. If distance is Far and value is High, then priority is Medium
    // 5. If distance is Close and value is Low, then priority is Medium
    // 6. If distance is Medium and value is Medium, then priority is Medium
    // 7. If distance is Medium and value is Low, then priority is Low
    // 8. If distance is Far and value is Medium, then priority is Low
    // 9. If distance is Far and value is Low, then priority is VeryLow
    
    float totalWeight = 0.0f;
    float weightedSum = 0.0f;
    
    // Rule 1: Close + High -> VeryHigh
    float close = GetDistanceMembership(input.distance, "Close");
    float high = GetValueMembership(input.preyValue, "High");
    float weight1 = close * high;
    if (weight1 > 0.0f) {
        totalWeight += weight1;
        weightedSum += weight1 * 0.9f; // VeryHigh = 0.9
    }
    
    // Rule 2: Close + Medium -> High
    float medium_val = GetValueMembership(input.preyValue, "Medium");
    float weight2 = close * medium_val;
    if (weight2 > 0.0f) {
        totalWeight += weight2;
        weightedSum += weight2 * 0.7f; // High = 0.7
    }
    
    // Rule 3: Medium + High -> High
    float medium_dist = GetDistanceMembership(input.distance, "Medium");
    float weight3 = medium_dist * high;
    if (weight3 > 0.0f) {
        totalWeight += weight3;
        weightedSum += weight3 * 0.7f; // High = 0.7
    }
    
    // Rule 4: Far + High -> Medium
    float far = GetDistanceMembership(input.distance, "Far");
    float weight4 = far * high;
    if (weight4 > 0.0f) {
        totalWeight += weight4;
        weightedSum += weight4 * 0.5f; // Medium = 0.5
    }
    
    // Rule 5: Close + Low -> Medium
    float low_val = GetValueMembership(input.preyValue, "Low");
    float weight5 = close * low_val;
    if (weight5 > 0.0f) {
        totalWeight += weight5;
        weightedSum += weight5 * 0.5f; // Medium = 0.5
    }
    
    // Rule 6: Medium + Medium -> Medium
    float weight6 = medium_dist * medium_val;
    if (weight6 > 0.0f) {
        totalWeight += weight6;
        weightedSum += weight6 * 0.5f; // Medium = 0.5
    }
    
    // Rule 7: Medium + Low -> Low
    float weight7 = medium_dist * low_val;
    if (weight7 > 0.0f) {
        totalWeight += weight7;
        weightedSum += weight7 * 0.3f; // Low = 0.3
    }
    
    // Rule 8: Far + Medium -> Low
    float weight8 = far * medium_val;
    if (weight8 > 0.0f) {
        totalWeight += weight8;
        weightedSum += weight8 * 0.3f; // Low = 0.3
    }
    
    // Rule 9: Far + Low -> VeryLow
    float weight9 = far * low_val;
    if (weight9 > 0.0f) {
        totalWeight += weight9;
        weightedSum += weight9 * 0.1f; // VeryLow = 0.1
    }
    
    // Defuzzification using weighted average
    if (totalWeight > 0.0f) {
        return weightedSum / totalWeight;
    }
    
    return 0.0f; // No applicable rules
}

// Distance Membership Functions
float DrawBall::GetDistanceMembership(float distance, const std::string& category) {
    if (category == "Close") {
        // Close: peak at 0, zero at 3
        if (distance <= 0.5f) return 1.0f;
        if (distance >= 3.0f) return 0.0f;
        return (3.0f - distance) / 2.5f;
    }
    else if (category == "Medium") {
        // Medium: peak at 3.5, zero at 1 and 6
        if (distance <= 1.0f || distance >= 6.0f) return 0.0f;
        if (distance <= 3.5f) return (distance - 1.0f) / 2.5f;
        return (6.0f - distance) / 2.5f;
    }
    else if (category == "Far") {
        // Far: peak at 7, zero at 4
        if (distance <= 4.0f) return 0.0f;
        if (distance >= 7.0f) return 1.0f;
        return (distance - 4.0f) / 3.0f;
    }
    return 0.0f;
}

// Value Membership Functions  
float DrawBall::GetValueMembership(int value, const std::string& category) {
    if (category == "Low") {
        // Low: peak at 5, zero at 8
        if (value == 5) return 1.0f;
        if (value >= 10) return 0.0f;
        if (value < 5) return 1.0f;
        return (10.0f - value) / 5.0f;
    }
    else if (category == "Medium") {
        // Medium: peak at 10, zero at 5 and 15
        if (value == 10) return 1.0f;
        if (value <= 5 || value >= 15) return 0.0f;
        if (value < 10) return (value - 5.0f) / 5.0f;
        return (15.0f - value) / 5.0f;
    }
    else if (category == "High") {
        // High: peak at 15, zero at 10
        if (value == 15) return 1.0f;
        if (value <= 10) return 0.0f;
        if (value > 15) return 1.0f;
        return (value - 10.0f) / 5.0f;
    }
    return 0.0f;
}

// Priority Membership Functions (for output)
float DrawBall::GetPriorityMembership(float priority, const std::string& category) {
    if (category == "VeryLow") {
        return (priority <= 0.2f) ? 1.0f : std::max(0.0f, (0.3f - priority) / 0.1f);
    }
    else if (category == "Low") {
        if (priority <= 0.2f || priority >= 0.4f) return 0.0f;
        if (priority <= 0.3f) return (priority - 0.2f) / 0.1f;
        return (0.4f - priority) / 0.1f;
    }
    else if (category == "Medium") {
        if (priority <= 0.4f || priority >= 0.6f) return 0.0f;
        if (priority <= 0.5f) return (priority - 0.4f) / 0.1f;
        return (0.6f - priority) / 0.1f;
    }
    else if (category == "High") {
        if (priority <= 0.6f || priority >= 0.8f) return 0.0f;
        if (priority <= 0.7f) return (priority - 0.6f) / 0.1f;
        return (0.8f - priority) / 0.1f;
    }
    else if (category == "VeryHigh") {
        return (priority >= 0.8f) ? 1.0f : std::max(0.0f, (priority - 0.7f) / 0.1f);
    }
    return 0.0f;
}

// Reset AI state for predators
void DrawBall::ResetAIState() {
    currentState = FSMState::SelectTarget;
    targetPrey = nullptr;
    lastTargetSelectionTime = 0.0f;
}

void DrawBall::Render(Shader* shader, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos) {
    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, position);
    modelMat = glm::scale(modelMat, glm::vec3(scale));

    shader->use();

    glUniform1i(glGetUniformLocation(shader->ID, "isbox"), 0);
    glUniform1i(glGetUniformLocation(shader->ID, "isRoom"), 0);

    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(proj));

    glUniform3f(glGetUniformLocation(shader->ID, "objColor"), color.x, color.y, color.z);
    glUniform3f(glGetUniformLocation(shader->ID, "ambientColor"), 0.3f, 0.3f, 0.3f);
    glUniform3f(glGetUniformLocation(shader->ID, "lightPos"), 2.0f, 4.0f, 2.0f);
    glUniform3f(glGetUniformLocation(shader->ID, "lightColor"), 0.8f, 0.8f, 0.8f);
    glUniform3f(glGetUniformLocation(shader->ID, "lightPos2"), -2.0f, 4.0f, -2.0f);
    glUniform3f(glGetUniformLocation(shader->ID, "lightColor2"), 0.6f, 0.6f, 0.6f);
    glUniform3f(glGetUniformLocation(shader->ID, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform1i(glGetUniformLocation(shader->ID, "light1Enabled"), light1Enabled);
    glUniform1i(glGetUniformLocation(shader->ID, "light2Enabled"), light2Enabled);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}
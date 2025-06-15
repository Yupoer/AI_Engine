#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>
#include "Shader.h"
#include "AABB.h"
#include "BoundingSphere.h"

// FSM States for Gray Predator
enum class FSMState {
    SelectTarget,
    ChaseTarget
};

// Fuzzy Logic structures for Purple Predator
struct FuzzyInput {
    float distance;
    int preyValue;
};

struct FuzzyOutput {
    float priority;
};

class DrawBall {
private:
    GLuint VAO;
    int vertexCount;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float scale;
    float gravity;
    glm::vec3 color;
    bool isPredator;
    BoundingSphere boundingBox; // 使用 BoundingSphere
    bool isStationary;
    int score; // 掠食者的積分
    int point; // 一般球的分數值
    
    // AI Engine variables
    FSMState currentState;
    DrawBall* targetPrey;
    float lastTargetSelectionTime;
    float predatorSpeed;

public:
    DrawBall(GLuint VAO, int vertexCount, float radius = 0.03f);
    ~DrawBall();

    void UpdateBoundingSphere();
    void Update(float deltaTime, const AABB& roomAABB);
    void Update(float deltaTime, const AABB& roomAABB, const std::vector<DrawBall*>& balls);
    void Render(Shader* shader, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos);
    
    // AI Engine methods
    void UpdateFSM(float deltaTime, const std::vector<DrawBall*>& preys);
    void UpdateFuzzyLogic(float deltaTime, const std::vector<DrawBall*>& preys);
    DrawBall* SelectTargetFSM(const std::vector<DrawBall*>& preys);
    DrawBall* SelectTargetFuzzy(const std::vector<DrawBall*>& preys);
    void ChaseTarget(float deltaTime);
    float CalculateFuzzyPriority(const FuzzyInput& input);
    float GetDistanceMembership(float distance, const std::string& category);
    float GetValueMembership(int value, const std::string& category);
    float GetPriorityMembership(float priority, const std::string& category);

    void SetPosition(const glm::vec3& pos) { position = pos; UpdateBoundingSphere(); }
    void SetVelocity(const glm::vec3& vel) { velocity = vel; isStationary = false; }
    void SetGravity(float g) { gravity = g; }
    void SetScale(float s) { scale = s; boundingBox.radius = s; UpdateBoundingSphere(); }
    void SetColor(const glm::vec3& c) { color = c; }
    void SetIsPredator(bool predator) { isPredator = predator; }
    void SetScore(int s) { score = s; }
    void SetPoint(int p) { point = p; }
    void SetPredatorSpeed(float speed) { predatorSpeed = speed; }
    void ResetAIState(); // Reset AI state for predators

    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetVelocity() const { return velocity; }
    float GetScale() const { return scale; }
    glm::vec3 GetColor() const { return color; }
    bool IsStationary() const { return isStationary; }
    bool IsPredator() const { return isPredator; }
    int GetScore() const { return score; }
    int GetPoint() const { return point; }
    FSMState GetCurrentState() const { return currentState; }
    DrawBall* GetTargetPrey() const { return targetPrey; }
};

extern bool light1Enabled;
extern bool light2Enabled;
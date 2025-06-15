#pragma once
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "model_data.h"
#include "DrawBall.h"
#include "AABB.h"
#include <vector>
#include <algorithm>

#pragma region Model Data

float roomVertices[] = {
    // 背面 (Z = -roomSize)
    -5.0f,  5.0f, -5.0f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f, // 左上
     5.0f,  5.0f, -5.0f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f, // 右上
     5.0f, -5.0f, -5.0f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f, // 右下
     5.0f, -5.0f, -5.0f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f, // 右下
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f, // 左下
    -5.0f,  5.0f, -5.0f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f, // 左上

    // 正面 (Z = roomSize)
    -5.0f, -5.0f,  5.0f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f, // 左下
     5.0f, -5.0f,  5.0f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f, // 右下
     5.0f,  5.0f,  5.0f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f, // 右上
     5.0f,  5.0f,  5.0f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f, // 右上
    -5.0f,  5.0f,  5.0f,  0.0f, 1.0f,  0.0f,  0.0f,  1.0f, // 左上
    -5.0f, -5.0f,  5.0f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f, // 左下

    // 左面 (X = -roomSize)
    -5.0f,  5.0f,  5.0f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f, // 右上
    -5.0f,  5.0f, -5.0f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f, // 左上
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f, // 左下
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f, // 左下
    -5.0f, -5.0f,  5.0f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f, // 右下
    -5.0f,  5.0f,  5.0f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f, // 右上

    // 右面 (X = roomSize)
     5.0f,  5.0f, -5.0f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f, // 左上
     5.0f,  5.0f,  5.0f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f, // 右上
     5.0f, -5.0f,  5.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f, // 右下
     5.0f, -5.0f,  5.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f, // 右下
     5.0f, -5.0f, -5.0f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f, // 左下
     5.0f,  5.0f, -5.0f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f, // 左上

    // 底面 (Y = -roomSize)
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f, // 左下
     5.0f, -5.0f, -5.0f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f, // 右下
     5.0f, -5.0f,  5.0f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f, // 右上
     5.0f, -5.0f,  5.0f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f, // 右上
    -5.0f, -5.0f,  5.0f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f, // 左上
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f, // 左下

    // 頂面 (Y = roomSize)
    -5.0f,  5.0f, -5.0f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f, // 左上
     5.0f,  5.0f, -5.0f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f, // 右上
     5.0f,  5.0f,  5.0f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f, // 右下
     5.0f,  5.0f,  5.0f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f, // 右下
    -5.0f,  5.0f,  5.0f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f, // 左下
    -5.0f,  5.0f, -5.0f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f  // 左上
};


#pragma endregion


#pragma region Input Declare

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}
#pragma endregion

unsigned int LoadImageToGPU(const char* filename, GLint internalFormat, GLenum format, int textureSlot) {
    unsigned int TexBuffer;
    glGenTextures(1, &TexBuffer);
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glBindTexture(GL_TEXTURE_2D, TexBuffer);


    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        printf("Texture %s loaded successfully: %d x %d\n", filename, width, height);
    }
    else {
        printf("Failed to load texture: %s\n", stbi_failure_reason());
    }
    stbi_image_free(data);
    return TexBuffer;
}

float x = 7.2f, y = 6.3f, z = 4.8f;
// Room AABB from -5 to 5 in all dimensions
AABB roomAABB(glm::vec3(x-10.0f, y-10.0f, z-10.0f), glm::vec3(x, y, z)); 

// Time tracking for physics
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Add physics controls for GUI
float gravityStrength = 9.8f;
float predatorSpeed = 5.0f; // 掠食者速度控制
bool resetBall = false;

std::vector<DrawBall*> balls; 
int maxBalls = 30;
int currentBalls = 1; 

void InitializeBalls(int count, GLuint VAO, int vertexCount) {
    // 只清除 isPredator 為 false 的球
    balls.erase(std::remove_if(balls.begin(), balls.end(), [](DrawBall* ball) {
        if (!ball->IsPredator()) {
            delete ball;
            return true;
        }
        return false;
    }), balls.end());

    // 獲取房間的邊界
    glm::vec3 roomMin = roomAABB.GetMin();
    glm::vec3 roomMax = roomAABB.GetMax();

    // 生成指定數量的非掠食者球
    for (int i = 0; i < count; i++) {
        float scale = 0.1f;
        DrawBall* ball = new DrawBall(VAO, vertexCount, scale);
        ball->SetScale(scale);
        
        // 隨機分配顏色和分數
        int colorType = rand() % 3;
        if (colorType == 0) {
            // 紅球 - 15 points
            ball->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
            ball->SetPoint(15);
        } else if (colorType == 1) {
            // 橙球 - 10 points
            ball->SetColor(glm::vec3(1.0f, 0.5f, 0.0f));
            ball->SetPoint(10);
        } else {
            // 黃球 - 5 points
            ball->SetColor(glm::vec3(1.0f, 1.0f, 0.0f));
            ball->SetPoint(5);
        }
        
        float x = roomMin.x + scale + (static_cast<float>(rand()) / RAND_MAX) * (roomMax.x - roomMin.x - 2.0f * scale);
        float z = roomMin.z + scale + (static_cast<float>(rand()) / RAND_MAX) * (roomMax.z - roomMin.z - 2.0f * scale);
        float y = roomMin.y + scale;
        glm::vec3 position(x, y, z);

        ball->SetPosition(position);
        
        // 根據分數設定初始速度
        float speed = 0.0f;
        if (ball->GetPoint() == 15) speed = 4.0f;      // 紅球
        else if (ball->GetPoint() == 10) speed = 3.0f; // 橙球
        else if (ball->GetPoint() == 5) speed = 2.0f;  // 黃球
        
        // 隨機方向的水平速度
        float randomX = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * speed;
        float randomZ = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * speed;
        ball->SetVelocity(glm::vec3(randomX, 0.0f, randomZ));
        
        ball->SetGravity(-gravityStrength);
        ball->SetIsPredator(false); // 標記為非掠食者
        balls.push_back(ball);
    }
}

void ResolveSphereCollision(DrawBall* ball1, DrawBall* ball2) {
    float randomFactor = 0.2f;
    glm::vec3 pos1 = ball1->GetPosition();
    glm::vec3 pos2 = ball2->GetPosition();
    float radius1 = ball1->GetScale();
    float radius2 = ball2->GetScale();

    glm::vec3 delta = pos2 - pos1;
    float distance = glm::length(delta);

    if (distance < 0.0001f) {
        delta = glm::vec3(static_cast<float>(rand()) / RAND_MAX - 0.5f);
        distance = glm::length(delta);
    }

    float overlap = (radius1 + radius2) - distance;

    if (overlap <= 0) {
        return;
    }


    glm::vec3 normal = delta / distance;

    float totalMass = 1.0f;
    float correction1 = overlap * 0.5f;
    float correction2 = overlap * 0.5f;

    ball1->SetPosition(pos1 - normal * correction1);
    ball2->SetPosition(pos2 + normal * correction2);

    pos1 = ball1->GetPosition();
    pos2 = ball2->GetPosition();

    glm::vec3 vel1 = ball1->GetVelocity();
    glm::vec3 vel2 = ball2->GetVelocity();

    float v1n = glm::dot(vel1, normal);
    float v2n = glm::dot(vel2, normal);

    if (v1n > v2n) {
        return;
    }

    float restitution = 0.6f;

    float v1nAfter = (v1n * (0.0f) + v2n * 2.0f) / 2.0f;
    float v2nAfter = (v2n * (0.0f) + v1n * 2.0f) / 2.0f;

    v1nAfter = v1n + restitution * (v1nAfter - v1n);
    v2nAfter = v2n + restitution * (v2nAfter - v2n);

    glm::vec3 v1nVector = normal * v1nAfter;
    glm::vec3 v2nVector = normal * v2nAfter;

    glm::vec3 v1t = vel1 - (normal * v1n);
    glm::vec3 v2t = vel2 - (normal * v2n);

    ball1->SetVelocity(v1t + v1nVector);
    ball2->SetVelocity(v2t + v2nVector);

    // 添加隨機擾動（只有在速度大於閾值時）

    if (glm::length(ball1->GetVelocity()) > 0.05f) {
        ball1->SetVelocity(ball1->GetVelocity() + glm::vec3(
            (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * randomFactor,
            0.0f,
            (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * randomFactor
        ));
    }
    if (glm::length(ball2->GetVelocity()) > 0.05f) {
        ball2->SetVelocity(ball2->GetVelocity() + glm::vec3(
            (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * randomFactor,
            0.0f,
            (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * randomFactor
        ));
    }

    // 移除了碰撞後的減速效果
}

float ceilingMixFactor = 0.5f;
float initialSpeedRange = 5.0f;
float groundFriction = 0.99f;

bool light1Enabled = true; // 第一個光源開關
bool light2Enabled = true; // 第二個光源開關



int main() {
    DrawBall* greyBall = nullptr;
    DrawBall* purpleBall = nullptr;

    #pragma region Open a Window
        if (!glfwInit()) {
            printf("Failed to initialize GLFW\n");
            return -1;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window = glfwCreateWindow(1600, 1200, "3D render", NULL, NULL);
        if (window == NULL) {
            const char* description;
            int code = glfwGetError(&description);
            printf("GLFW Error %d: %s\n", code, description);
            glfwTerminate();
            return -1;
        }

        glfwMakeContextCurrent(window);
        //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  

        // init GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            glfwTerminate();
            return -1;
        }

        glViewport(0, 0, 1600, 1200);
        glEnable(GL_DEPTH_TEST);
    #pragma endregion
    
    #pragma region Init ImGui
    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); 
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 400 core"); 
    #pragma endregion

    #pragma region Init Shader Program
    // load vertex and fragment shader
    Shader* myShader = new Shader("vertexShaderSource.vert", "fragmentShaderSource.frag");
    
    #pragma endregion
    
   

    #pragma region Init and load Model to VAO & VBO
    // create & bind cube VAO & VBO
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // set attrib pointer 
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);   
    glEnableVertexAttribArray(6);
    //glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    //glEnableVertexAttribArray(7);
    glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); 
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float))); 
    glEnableVertexAttribArray(9);

    
    // room VAO & VBO
    unsigned int roomVAO, roomVBO;
    glGenVertexArrays(1, &roomVAO);
    glBindVertexArray(roomVAO);
    
    glGenBuffers(1, &roomVBO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roomVertices), roomVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);  
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); 
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float))); 
    glEnableVertexAttribArray(9);
    #pragma endregion

    #pragma region Init and Load Texture
    unsigned int TexBufferA;
    unsigned int TexBufferB;
    TexBufferA = LoadImageToGPU("picSource/grid.jpg", GL_RGB, GL_RGB, 0);
    TexBufferB = LoadImageToGPU("picSource/container.jpg", GL_RGB, GL_RGB, 3);
    #pragma endregion  

    #pragma region Init Camera
    glm::vec3 position = { -4.0f, 1.0f, -4.0f }; //-4.0f, 3.0f, -4.0f
    glm::vec3 worldup = { 0.0f, 1.0f, 0.0f };
    Camera camera(position, glm::radians(0.0f), glm::radians(0.0f), worldup);
    Camera camera2(glm::vec3(0.0f, 5.0f, 0.0f), glm::radians(-90.0f), glm::radians(0.0f), worldup); // 頂視圖
    #pragma endregion

    #pragma region Prepare MVP(model view proj) Matrices
    glm::mat4 viewMat = glm::mat4(1.0f);
    glm::mat4 viewMat2 = glm::mat4(1.0f);
    viewMat = camera.GetViewMatrix();
    viewMat2 = camera2.GetViewMatrix();

    glm::mat4 modelMat = glm::mat4(1.0f);
    //modelMat = glm::rotate(modelMat, glm::radians(45.0f), glm::vec3(0.0f, 0.5f, 1.0f));

    glm::mat4 projMat = glm::mat4(1.0f);
    // 透視投影（FOV 45 度，寬高比 1600/1200，近裁剪面 0.1，遠裁剪面 100）
    projMat = glm::perspective(glm::radians(60.0f), 1600.0f / 1200.0f, 0.1f, 100.0f);

    // 正交投影（其他三個攝影機）
    glm::mat4 orthoProjMat = glm::mat4(1.0f);
    float aspectRatio = 800.0f / 600.0f; // 視口寬高比
    float width = 5.0f; // 房間一半寬度
    float height = width / aspectRatio; // 根據比例計算高度
    orthoProjMat = glm::ortho(-width, width, -height, height, 0.1f, 100.0f);
    #pragma endregion
    
    // Time initialization
    lastFrame = glfwGetTime();
    
    // 初始化受 ImGui 控制的球
    InitializeBalls(currentBalls, VAO, vertexCount);

    // 創建兩顆掠食者球
    // 灰色球
    float greyBallScale = 0.1f;
    greyBall = new DrawBall(VAO, vertexCount, greyBallScale);
    greyBall->SetScale(greyBallScale);
    greyBall->SetPosition(glm::vec3(-2.0f, roomAABB.GetMin().y + 0.1f, -2.0f));
    greyBall->SetVelocity(glm::vec3(0.0f, 0.0f, 0.0f)); // 掠食者速度為0
    greyBall->SetColor(glm::vec3(0.5f, 0.5f, 0.5f));
    greyBall->SetGravity(-gravityStrength);
    greyBall->SetIsPredator(true); // 設為掠食者
    greyBall->SetScore(0); // 初始分數為0
    greyBall->SetPredatorSpeed(predatorSpeed); // 設定掠食者速度
    balls.push_back(greyBall);

    // 紫色球
    float purpleBallScale = 0.1f;
    purpleBall = new DrawBall(VAO, vertexCount, purpleBallScale);
    purpleBall->SetScale(purpleBallScale);
    purpleBall->SetPosition(glm::vec3(2.0f, roomAABB.GetMin().y + 0.1f, 2.0f));
    purpleBall->SetVelocity(glm::vec3(0.0f, 0.0f, 0.0f)); // 掠食者速度為0
    purpleBall->SetColor(glm::vec3(0.5f, 0.0f, 0.5f));
    purpleBall->SetGravity(-gravityStrength);
    purpleBall->SetIsPredator(true); // 設為掠食者
    purpleBall->SetScore(0); // 初始分數為0
    purpleBall->SetPredatorSpeed(predatorSpeed); // 設定掠食者速度
    balls.push_back(purpleBall);

    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window);
            
        // Clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        #pragma region ImGui Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowPos(ImVec2(10, 10));
        ImGui::SetWindowSize(ImVec2(300, 400));

        // 相機控制（僅控制主攝影機）
        ImGui::Text("Adjust Main Camera Position (Top-Left View)");
        ImGui::SliderFloat3("Camera Position", &camera.Position[0], -10.0f, 10.0f);
        viewMat = camera.GetViewMatrix();

        ImGui::Text("Adjust Camera Pitch and Yaw");
        float pitch_deg = glm::degrees(camera.Pitch);
        float yaw_deg = glm::degrees(camera.Yaw);
        bool camera_updated = false;
        if (ImGui::SliderFloat("Pitch", &pitch_deg, -89.0f, 89.0f)) {
            camera.Pitch = glm::radians(pitch_deg);
            camera_updated = true;
        }
        if (ImGui::SliderFloat("Yaw", &yaw_deg, -180.0f, 180.0f)) {
            camera.Yaw = glm::radians(yaw_deg);
            camera_updated = true;
        }
        if (camera_updated) {
            camera.UpdateCameraVectors();
            viewMat = camera.GetViewMatrix();
        }

        ImGui::Separator();
        ImGui::Text("Light Controls");
        ImGui::Checkbox("Light 1 Enabled", &light1Enabled);
        ImGui::Checkbox("Light 2 Enabled", &light2Enabled);

        // 物理控制
        ImGui::Separator();
        ImGui::Text("Physics Controls");
        
        if (ImGui::SliderFloat("Gravity", &gravityStrength, 0.0f, 20.0f)) {
            for (auto ball : balls) {
                ball->SetGravity(-gravityStrength);
            }
        }
        
        // 球數量控制
        int oldBallCount = currentBalls;
        if (ImGui::SliderInt("Ball Count", &currentBalls, 1, maxBalls)) {
            InitializeBalls(currentBalls, VAO, vertexCount);
        }

        // 掠食者速度控制
        if (ImGui::SliderFloat("Predator Speed", &predatorSpeed, 1.0f, 10.0f)) {
            // 同步到所有掠食者
            for (auto ball : balls) {
                if (ball->IsPredator()) {
                    ball->SetPredatorSpeed(predatorSpeed);
                }
            }
        }

        if (ImGui::Button("Reset Balls")) {
            glm::vec3 roomMin = roomAABB.GetMin();
            glm::vec3 roomMax = roomAABB.GetMax();
            for (auto ball : balls) {
                float scale = ball->GetScale();
                float x = roomMin.x + scale + (static_cast<float>(rand()) / RAND_MAX) * (roomMax.x - roomMin.x - 2.0f * scale);
                float z = roomMin.z + scale + (static_cast<float>(rand()) / RAND_MAX) * (roomMax.z - roomMin.z - 2.0f * scale);
                float y = roomMin.y + scale;
                ball->SetPosition(glm::vec3(x, y, z));
                // 如果是掠食者，重設分數和AI狀態
                if (ball->IsPredator()) {
                    ball->SetScore(0);
                    ball->SetVelocity(glm::vec3(0.0f));
                    ball->ResetAIState(); // 重置AI狀態
                } else {
                    // 重設一般球的速度
                    float speed = 0.0f;
                    if (ball->GetPoint() == 15) speed = 4.0f;      // 紅球
                    else if (ball->GetPoint() == 10) speed = 3.0f; // 橙球
                    else if (ball->GetPoint() == 5) speed = 2.0f;  // 黃球
                    
                    float randomX = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * speed;
                    float randomZ = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * speed;
                    ball->SetVelocity(glm::vec3(randomX, 0.0f, randomZ));
                }
            }
        }

        // 顯示分數和AI狀態
        ImGui::Separator();
        ImGui::Text("Scores & AI Status:");
        for (auto ball : balls) {
            if (ball->IsPredator()) {
                glm::vec3 color = ball->GetColor();
                if (color.r > 0.4f && color.g > 0.4f && color.b > 0.4f) {
                    // Gray Predator (FSM)
                    ImGui::Text("Grey Predator (FSM) Score: %d", ball->GetScore());
                    std::string stateStr = (ball->GetCurrentState() == FSMState::SelectTarget) ? "SelectTarget" : "ChaseTarget";
                    ImGui::Text("  State: %s", stateStr.c_str());
                    if (ball->GetTargetPrey() != nullptr) {
                        ImGui::Text("  Target: Point %d", ball->GetTargetPrey()->GetPoint());
                    } else {
                        ImGui::Text("  Target: None");
                    }
                } else {
                    // Purple Predator (Fuzzy Logic)
                    ImGui::Text("Purple Predator (Fuzzy) Score: %d", ball->GetScore());
                    if (ball->GetTargetPrey() != nullptr) {
                        ImGui::Text("  Target: Point %d", ball->GetTargetPrey()->GetPoint());
                    } else {
                        ImGui::Text("  Target: None");
                    }
                }
            }
        }

        ImGui::Text("Camera Pitch: %.2f degrees", glm::degrees(camera.Pitch));
        ImGui::Text("Camera Yaw: %.2f degrees", glm::degrees(camera.Yaw));
        
        ImGui::End();
        #pragma endregion
    
        // 啟用剪裁測試
        glEnable(GL_SCISSOR_TEST);

        // 視口 1：左上（主攝影機，使用透視投影）
        glViewport(0, 600, 800, 600);
        glScissor(0, 600, 800, 600);
        glClear(GL_DEPTH_BUFFER_BIT);
        #pragma region Create room
        modelMat = glm::mat4(1.0f);

        myShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexBufferA);
        glUniform1i(glGetUniformLocation(myShader->ID, "roomTex"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 0);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat)); // 透視投影

        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);

        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        #pragma endregion

        

        for (auto ball : balls) {
            ball->Render(myShader, viewMat, projMat, camera.Position);
        }

        // 視口 2：右上（頂視圖，使用正交投影）
        glViewport(800, 600, 800, 600);
        glScissor(800, 600, 800, 600);
        glClear(GL_DEPTH_BUFFER_BIT);
        #pragma region Create room
        modelMat = glm::mat4(1.0f);

        myShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexBufferA);
        glUniform1i(glGetUniformLocation(myShader->ID, "roomTex"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 0);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat2));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(orthoProjMat)); // 正交投影

        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera2.Position.x, camera2.Position.y, camera2.Position.z);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);

        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        #pragma endregion

        

        for (auto ball : balls) {
            ball->Render(myShader, viewMat2, orthoProjMat, camera2.Position);
        }

        // 禁用剪裁測試
        glDisable(GL_SCISSOR_TEST);

        // 收集掠食者和獵物的列表
        std::vector<DrawBall*> predators;
        std::vector<DrawBall*> preys;
        for (auto ball : balls) {
            if (ball->IsPredator()) {
                predators.push_back(ball);
            } else {
                preys.push_back(ball);
            }
        }
        
        for (auto ball : balls) {
            ball->Update(deltaTime, roomAABB, balls);
        }
            
            // 碰撞檢測和處理
            std::vector<DrawBall*> ballsToRemove;
            
            for (size_t i = 0; i < balls.size(); i++) {
                for (size_t j = i + 1; j < balls.size(); j++) {
                    glm::vec3 pos1 = balls[i]->GetPosition();
                    glm::vec3 pos2 = balls[j]->GetPosition();
                    float radius1 = balls[i]->GetScale();
                    float radius2 = balls[j]->GetScale();
                    
                    if (AABB::SphereToSphere(pos1, radius1, pos2, radius2)) {
                        // 檢查是否為掠食者與一般球的碰撞
                        bool isPredatorPreyCollision = false;
                        DrawBall* predator = nullptr;
                        DrawBall* prey = nullptr;
                        
                        if (balls[i]->IsPredator() && !balls[j]->IsPredator()) {
                            predator = balls[i];
                            prey = balls[j];
                            isPredatorPreyCollision = true;
                        } else if (!balls[i]->IsPredator() && balls[j]->IsPredator()) {
                            predator = balls[j];
                            prey = balls[i];
                            isPredatorPreyCollision = true;
                        }
                        
                        if (isPredatorPreyCollision) {
                            // 掠食者吃掉獵物
                            predator->SetScore(predator->GetScore() + prey->GetPoint());
                            // 標記要移除的球
                            if (std::find(ballsToRemove.begin(), ballsToRemove.end(), prey) == ballsToRemove.end()) {
                                ballsToRemove.push_back(prey);
                            }
                        } else {
                            // 一般的球與球碰撞
                            ResolveSphereCollision(balls[i], balls[j]);
                        }
                    }
                }
            }
            
            // 移除被吃掉的球
            for (auto ballToRemove : ballsToRemove) {
                auto it = std::find(balls.begin(), balls.end(), ballToRemove);
                if (it != balls.end()) {
                    delete *it;
                    balls.erase(it);
                }
            }
        
        // 渲染所有球
        for (auto ball : balls) {
            ball->Render(myShader, viewMat, projMat, camera.Position);
        }

        // 檢查 OpenGL 錯誤
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL Error: " << err << std::endl;
        }
        
        #pragma region Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #pragma endregion

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理
    for (auto ball : balls) {
        delete ball;
    }
    balls.clear();

    //Exit program
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
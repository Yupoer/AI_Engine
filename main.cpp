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
#include "ball.h"
#include "vector"
#include "irregular.h"
#include "room.h"
#include "object.h"
#include <thread>
#include <chrono>
#include "physicManager.h"

// AABB 線框的頂點數據
float aabbVertices[] = {
    // 前面
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    // 後面
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f
};

unsigned int aabbIndices[] = {
    // 前面
    0, 1, 1, 2, 2, 3, 3, 0,
    // 後面
    4, 5, 5, 6, 6, 7, 7, 4,
    // 連接前後
    0, 4, 1, 5, 2, 6, 3, 7
};

#pragma region Input Declare
// Forward declare apply_impulse_to_objects function if it's defined later or in another file
// For now, we will define a simple version or handle input directly in processInput
void apply_external_force_to_objects(Object& ball, Object& irregular, bool& shouldApply);

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    // Placeholder for applying force - e.g., press 'J' to apply impulse
    // static bool j_key_pressed = false;
    // if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && !j_key_pressed) {
    //     j_key_pressed = true;
    //     // apply_external_force_to_objects(ballObj, irregularObj, true); // This needs ballObj and irregularObj to be accessible
    // }
    // if (glfwGetKey(window, GLFW_KEY_J) == GLFW_RELEASE) {
    //    j_key_pressed = false;
    // }
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

// Time tracking for physics
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Add physics controls for GUI
bool pausePhysics = false;
float gravityStrength = 9.8f;
bool resetBall = false;
bool resetIrregular = false;

// Removed ball and mini room related variables
bool light1Enabled = true; // 第一個光源開關
bool light2Enabled = true; // 第二個光源開關

// 修改 Object 初始化，加入阻尼參數
Object irregularObj(
    glm::vec3(irregularMin[0], irregularMin[1], irregularMin[2]),  // localMin
    glm::vec3(irregularMax[0], irregularMax[1], irregularMax[2]),  // localMax
    glm::vec3(7.0f, 2.0f, 7.0f),                                   // startPos - Y raised to 2.0f to be above floor
    1.0f,                                                          // 質量
    0.8f,                                                          // 摩擦係數
    0.3f,                                                          // 彈性係數 (was 0.2f, now 0.3f for irr)
    0.75f                                                          // 阻尼係數 (was 0.85f, now 0.75f for irr - larger drag)
);

// 修改球的初始化，使用新的球體建構子，加入阻尼參數
Object ballObj(
    0.7f,                         // 半徑
    glm::vec3(7.0f, 6.0f, 7.0f),  // 初始位置 - Y set to 0.51f (radius + epsilon) to rest on floor
    0.7f,                         // 質量
    0.6f,                         // 摩擦係數
    0.4f,                         // 彈性係數 (was 0.4f, now 0.8f for ball - higher bounce)
    0.98f                         // 阻尼係數 (was 0.98f - less drag for ball)
);

// float gravity = 9.8f; // This global var might be unused if physicManager.gravityAcceleration is used

PhysicManager physicManager(gravityStrength);

#pragma region Helper Function to Create VAO and VBO
void setupModelBuffers(unsigned int& VAO, unsigned int& VBO, const float* vertices, int vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertexCount * 8, vertices, GL_STATIC_DRAW);
    
    // 位置屬性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 紋理座標屬性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // 法線屬性
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
#pragma endregion

int main() {
    // 設置初始旋轉（繞 Y 軸旋轉 45 度）
    glm::quat initialRotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 0.0f));
    irregularObj.initRotation = initialRotation;
    irregularObj.rotation = initialRotation;

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

    // Room AABB 初始化
    AABB roomAABB(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 10.0f, 10.0f)); 

    #pragma region Init and load Model to VAO & VBO
    // room VAO & VBO
    unsigned int roomVAO, roomVBO;
    setupModelBuffers(roomVAO, roomVBO, roomVertices, sizeof(roomVertices) / sizeof(roomVertices[0]));
    
    // 為 irregularVertices 物件創建 VAO 和 VBO
    unsigned int irregularVAO, irregularVBO;
    setupModelBuffers(irregularVAO, irregularVBO, irregularVertices, irregularCount);
    
    // 為 ball 物件創建 VAO 和 VBO
    unsigned int ballVAO, ballVBO;
    setupModelBuffers(ballVAO, ballVBO, ballVertices, ballCount);

    // 創建 AABB 線框的 VAO 和 VBO
    unsigned int aabbVAO, aabbVBO, aabbEBO;
    glGenVertexArrays(1, &aabbVAO);
    glGenBuffers(1, &aabbVBO);
    glGenBuffers(1, &aabbEBO);

    glBindVertexArray(aabbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, aabbVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(aabbVertices), aabbVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aabbEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(aabbIndices), aabbIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    #pragma endregion

    #pragma region Init and Load Texture
    unsigned int TexBufferA;
    unsigned int TexBufferB;
    TexBufferA = LoadImageToGPU("picSource/grid.jpg", GL_RGB, GL_RGB, 0);
    TexBufferB = LoadImageToGPU("picSource/container.jpg", GL_RGB, GL_RGB, 3);
    #pragma endregion  

    #pragma region Init Camera
    glm::vec3 position = { 1.0f, 9.0f, 1.0f };
    glm::vec3 worldup = { 0.0f, 1.0f, 0.0f };
    Camera camera(position, glm::radians(0.0f), glm::radians(0.0f), worldup);
    #pragma endregion

    #pragma region Prepare MVP(model view proj) Matrices
    glm::mat4 viewMat = glm::mat4(1.0f);
    viewMat = camera.GetViewMatrix();

    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f)); // 將房間中心(5,5,5)，使範圍為(0,0,0)到(10,10,10)

    glm::mat4 projMat = glm::mat4(1.0f);
    // 透視投影（FOV 45 度，寬高比 1600/1200，近裁剪面 0.1，遠裁剪面 100）
    projMat = glm::perspective(glm::radians(60.0f), 1600.0f / 1200.0f, 0.1f, 100.0f);
    #pragma endregion
    
    // Time initialization
    lastFrame = glfwGetTime();

    // 確保初始速度為零，以實現初始靜止
    ballObj.SetVelocity(glm::vec3(0.0f));
    ballObj.angularVelocity = glm::vec3(0.0f);
    irregularObj.SetVelocity(glm::vec3(0.0f));
    irregularObj.angularVelocity = glm::vec3(0.0f);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window);
            
        // Clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        #pragma region ImGui Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::SetWindowPos(ImVec2(10, 10));
        ImGui::SetWindowSize(ImVec2(300, 400));

        ImGui::Text("Adjust Main Camera Position (Top-Left View)");
        ImGui::SliderFloat3("Camera Position", &camera.Position[0], -10.0f, 15.0f);
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
        /*
        ImGui::Separator();
        ImGui::Text("AABB Controls");
        bool showAABB = AABB::GetShowCollisionVolumes();
        if (ImGui::Checkbox("Show AABB Wireframe", &showAABB)) {
            AABB::SetShowCollisionVolumes(showAABB);
        }
        */
        ImGui::Text("Camera Pitch: %.2f degrees", glm::degrees(camera.Pitch));
        ImGui::Text("Camera Yaw: %.2f degrees", glm::degrees(camera.Yaw));
        
        ImGui::Separator();
        ImGui::Text("Physics Controls");
        ImGui::Checkbox("Pause Physics", &pausePhysics);
        ImGui::SliderFloat("Gravity", &gravityStrength, 0.0f, 20.0f);
        physicManager.gravityAcceleration = gravityStrength;
        ImGui::SliderFloat("Angular Drag", &physicManager.angularDragCoefficient, 0.0f, 5.0f, "%.3f");
        ImGui::SliderFloat("Ball Restitution", &ballObj.restitution, 0.0f, 1.0f, "%.2f");

        if (ImGui::Button("Apply Upward Impulse to All Objects")) {
            glm::vec3 impulse(4.0f, 10.0f, 0.0f); // Impulse vector (adjust magnitude as needed, e.g., 2.0f for noticeable effect)
            // Apply to ballObj
            ballObj.applyImpulse(impulse, ballObj.getWorldCenterOfMass());
            // Apply to irregularObj
            irregularObj.applyImpulse(impulse, irregularObj.getWorldCenterOfMass());
        }

        if (ImGui::Button("Reset")) {
            resetBall = true;
            ballObj.reset();
            resetIrregular = true;
            irregularObj.reset();
        }
        
        ImGui::End();
        #pragma endregion
    
        // 設置視口為整個窗口
        glViewport(0, 0, 1600, 1200);
        myShader->use();
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);

        #pragma region Create room   
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexBufferA);
        glUniform1i(glGetUniformLocation(myShader->ID, "roomTex"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 1);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat)); // 透視投影
        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        #pragma endregion

        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isAABB"), 0);

        #pragma region Draw Irregular Object
        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.8f, 0.2f, 0.2f); // 紅色
        glm::mat4 irregularModelMat = glm::translate(glm::mat4(1.0f), irregularObj.GetPosition());
        irregularModelMat = irregularModelMat * glm::mat4_cast(irregularObj.GetRotation());

        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(irregularModelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));
        glBindVertexArray(irregularVAO);
        glDrawArrays(GL_TRIANGLES, 0, irregularCount);

        // 繪製 OBB
        glUseProgram(myShader->ID);
        OBB::DrawOBB(irregularObj.GetOBB(), myShader->ID, aabbVAO, AABB::GetShowCollisionVolumes());
        #pragma endregion

        #pragma region Draw Ball Object
        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.8f, 0.2f, 0.2f); 
        glm::mat4 ballModelMat = glm::translate(glm::mat4(1.0f), ballObj.GetPosition());
        ballModelMat = ballModelMat * glm::mat4_cast(ballObj.GetRotation());
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(ballModelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));
        glBindVertexArray(ballVAO);
        glDrawArrays(GL_TRIANGLES, 0, ballCount);

        // 繪製 Bounding Sphere
        glUseProgram(myShader->ID);
        // Draw the bounding sphere for the ball object
        AABB::DrawWireSphere(ballObj.GetBoundingSphere(), myShader->ID, aabbVAO, AABB::GetShowCollisionVolumes());
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL Error after drawing collision volumes: " << err << std::endl;
        }
        // Draw the OBB for the irregular object
        OBB::DrawOBB(irregularObj.GetOBB(), myShader->ID, aabbVAO, AABB::GetShowCollisionVolumes());
        AABB::DrawAABB(roomAABB, myShader->ID, aabbVAO, AABB::GetShowCollisionVolumes());
        glUniform1i(glGetUniformLocation(myShader->ID, "isAABB"), 0);
        #pragma endregion
        
        glBindVertexArray(0);
        
        // 檢查 OpenGL 錯誤
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL Error: " << err << std::endl;
        }
        
        #pragma region Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #pragma endregion

        // 物理模擬交給 PhysicManager
        std::vector<Object*> objects = { &irregularObj, &ballObj };
        physicManager.pausePhysics = pausePhysics; // 同步暫停狀態
        physicManager.update(objects, roomAABB, deltaTime);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // 限制 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    // Exit program
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
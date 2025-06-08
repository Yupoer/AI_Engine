#include "Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 worldup) {
    Position = position;
    Worldup = worldup;
    Forward = glm::normalize(target - position);
    Right = glm::normalize(glm::cross(Forward, Worldup));
    Up = glm::normalize(glm::cross(Forward, Right));
}


Camera::Camera(glm::vec3 position, float pitch, float yaw, glm::vec3 worldup) {
 
    Position = position;
    Worldup = worldup;
    Pitch = pitch;
    Yaw = yaw;
    Forward.x = glm::cos(Pitch) * glm::cos(Yaw);
    Forward.y = glm::sin(Pitch);
    Forward.z = glm::cos(Pitch) * glm::sin(Yaw);
    Right = glm::normalize(glm::cross(Forward, Worldup));
    Up = glm::normalize(glm::cross(Forward, Right));
}

glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(Position, Position + Forward, Up);
}

void Camera::UpdateCameraVectors() {
    Forward.x = cos(Yaw) * cos(Pitch);
    Forward.y = sin(Pitch);
    Forward.z = sin(Yaw) * cos(Pitch);
    Forward = glm::normalize(Forward);

    Right = glm::normalize(glm::cross(Forward, Worldup));
    Up = glm::normalize(glm::cross(Right, Forward));
}

Camera::~Camera(){

}

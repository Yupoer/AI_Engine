#ifndef DB5E61E5_160E_40E5_B3D5_ED6A3074A890
#define DB5E61E5_160E_40E5_B3D5_ED6A3074A890

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera{
public:
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 worldup);
    Camera(glm::vec3 position, float pitch, float yaw, glm::vec3 worldup);
    ~Camera();

    glm::vec3 Position;
    glm::vec3 Forward;
    glm::vec3 Right;
    glm::vec3 Up;
    glm::vec3 Worldup;
    float Pitch;
    float Yaw;

    glm::mat4 GetViewMatrix();

    void UpdateCameraVectors();
};





#endif /* DB5E61E5_160E_40E5_B3D5_ED6A3074A890 */

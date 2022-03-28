
#include "camera.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>


extern GLFWwindow *window;

Camera::Camera() : Camera(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1)) {}

Camera::Camera(glm::vec3 position, glm::vec3 lookingDir) {

    m_Position = position;
    lookAt(position + lookingDir);
    m_FOV = 90.0f;

	m_PerspectiveMatrix = glm::perspective(m_FOV, (float)5/(float)3, 0.1f, 100.0f);
}

glm::vec3 Camera::getLookingDir() const {
    return m_LookingDir;
}


glm::vec3 Camera::getSideDir() const {
    return glm::normalize(glm::cross(m_LookingDir, glm::vec3(0.0f, 1.0f, 0.0f)));
}

void Camera::lookAt(glm::vec3 position) {

    m_LookingDir = glm::normalize(position - m_Position);

}

glm::mat3 Camera::getLookingMatrix() const {
    glm::vec3 sideDir = glm::normalize(glm::cross(m_LookingDir, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 upDir = glm::normalize(glm::cross(sideDir, m_LookingDir));

    return glm::mat3 (
        sideDir,  upDir,    m_LookingDir
    );
}

void Camera::rotate(float angle, glm::vec3 around) {
    m_LookingDir = glm::rotate(m_LookingDir, angle, around);
}

void Camera::rotateX(float angle) {
    glm::vec3 newLook = getLookingMatrix() * glm::vec3(0.0f, sin(angle), cos(angle));

    if (!(fabs(newLook.y) > 0.98f)) {
        m_LookingDir = newLook;
    }
}

void Camera::rotateY(float angle) {

    glm::vec3 newLook = getLookingMatrix() * glm::vec3(sin(angle), 0, cos(angle));
    
    if (!(fabs(newLook.y) > 0.98f)) {
        m_LookingDir = newLook;
    }
}

glm::mat4 Camera::getViewMatrix() const {

    glm::vec3 sideDir;
    glm::vec3 upDir;
    glm::vec3 transPos;

    sideDir = glm::normalize(glm::cross(m_LookingDir, glm::vec3(0.0f, 1.0f, 0.0f)));

    upDir = glm::normalize(glm::cross(sideDir, m_LookingDir));

    transPos = glm::vec3(-glm::dot(sideDir, m_Position), -glm::dot(upDir, m_Position), glm::dot(m_LookingDir, m_Position));

    return glm::mat4 (

        sideDir.x,  upDir.x,    -m_LookingDir.x,    0.0f,
        sideDir.y,  upDir.y,    -m_LookingDir.y,    0.0f,
        sideDir.z,  upDir.z,    -m_LookingDir.z,    0.0f,
        transPos.x, transPos.y,  transPos.z,        1.0f

    );
}

glm::mat4 Camera::getPerspectiveMatrix() const {
    return m_PerspectiveMatrix;
}

void Camera::updatePerspectiveMatrix(float FOV) {
    updatePerspectiveMatrix(FOV, 0.1f, 100.0f);
}

void Camera::updatePerspectiveMatrix(float FOV, float near, float far) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    m_FOV = FOV;
    m_PerspectiveMatrix = glm::perspectiveFov(m_FOV, (float)width, (float)height, near, far);
}

glm::vec3 Camera::getPos() const {
    return m_Position;
}

float* Camera::getPosValuePtr() {
    return glm::value_ptr(m_Position);
}

void Camera::setPos(glm::vec3 position) {
    m_Position = position;
}

void Camera::move(glm::vec3 mag) {
    m_Position += mag;
}

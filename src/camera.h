#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include "common.h"
#include "GL/glew.h"

class Camera {

public:

    Camera();
    Camera(glm::vec3 position, glm::vec3 lookingDir);

    glm::vec3 getLookingDir() const;
    glm::vec3 getSideDir() const;

    glm::mat3 getLookingMatrix() const;
    glm::mat4 getViewMatrix() const;
    glm::mat4 getPerspectiveMatrix() const;

    glm::vec3 getPos() const;
    float* getPosValuePtr();
    void setPos(glm::vec3 position);
    void move(glm::vec3 mag);

    void updatePerspectiveMatrix(float FOV);
    void updatePerspectiveMatrix(float FOV, float near, float far);

    void lookAt(glm::vec3 position);

    void rotate(float angle, glm::vec3 around);
    void rotateX(float angle);
    void rotateY(float angle);

private:

    float m_FOV;

    glm::vec3 m_Position;
    glm::vec3 m_LookingDir;

    glm::mat4 m_PerspectiveMatrix;

}; 

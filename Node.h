#pragma once
#include <glm/vec3.hpp>

#include <vector>
#include <atomic>

struct Connection;

class Node {
public:

    glm::vec3 velocity = glm::vec3(0);
    glm::vec3 force = glm::vec3(0);
    glm::vec3 position = glm::vec3(0);

    float mass = 0.004f;
    bool islocked = false;

    std::vector<Connection*> connections = {};

    bool imguiopen = false;
    bool drawconnections = false;

};
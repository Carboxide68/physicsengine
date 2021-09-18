#pragma once
#include <glm/vec3.hpp>

#include <vector>
#include <atomic>

struct Connection;

class Node {
public:

    Node() {
        islocked.store(false);
        drawconnections.store(false);
        imguiopen.store(false);
    }

    Node(const Node& other) {
        position = other.position;
        velocity = other.velocity;
        force = other.force;
        connections = other.connections;
        mass = other.mass;
        islocked.store(other.islocked.load());
        drawconnections.store(other.drawconnections.load());
        imguiopen.store(other.imguiopen.load());
    }

    void operator=(const Node& other) {
        position = other.position;
        velocity = other.velocity;
        force = other.force;
        connections = other.connections;
        mass = other.mass;
        islocked.store(other.islocked.load());
        drawconnections.store(other.drawconnections.load());
        imguiopen.store(other.imguiopen.load());
    }

    glm::vec3 velocity = glm::vec3(0);
    glm::vec3 force = glm::vec3(0);
    glm::vec3 position = glm::vec3(0);

    float mass = 0.004f;
    std::atomic_bool islocked;

    std::vector<uint> connections = {};

    std::atomic_bool imguiopen;
    std::atomic_bool drawconnections;

};
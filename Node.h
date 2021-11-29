#pragma once
#include <glm/vec3.hpp>

#include <vector>
#include <atomic>

struct NodeData {

    NodeData() : is_locked(false) {}

    NodeData(const NodeData& other) {
        is_locked.store(other.is_locked.load());
    }

    NodeData& operator=(const NodeData& other) {
        if (this == &other) return *this;
        node = other.node;
        is_locked.store(other.is_locked.load());
        draw_connections = other.draw_connections;
        imgui_open = other.imgui_open;
        return *this;
    }

    uint node;

    std::atomic<bool> is_locked;
    bool draw_connections = false;
    bool imgui_open = false;

};

struct Node {

    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
	glm::vec3 force;

    float mass;
    std::vector<uint> connections = {};

};

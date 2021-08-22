#pragma once

#include "Node.h"
#include <glm/geometric.hpp>

struct Connection {
    Node* node1 = nullptr;
    Node* node2 = nullptr;

    double neutrallen = 0.0;

    inline float GetLength() const noexcept {
        if (!node1 || !node2) return 0.0; 
        return glm::length(node1->position - node2->position);
    }
};
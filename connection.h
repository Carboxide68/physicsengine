#pragma once

#include "Node.h"
#include <glm/geometric.hpp>

struct Connection {
    uint node1 = (uint)-1;
    uint node2 = (uint)-1;

    double neutrallen = 0.0;

    inline float GetLength(const std::vector<Node>& nodes) const noexcept {
        if (node1 == (uint)-1 || node2 == (uint)-1) return 0.0; 
        return glm::length(nodes[node1].position - nodes[node2].position);
    }
};
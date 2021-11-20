#include "SoftBody.h"
#include "globals.h"

Ref<SoftBody> SoftBody::Create(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> connections, std::vector<std::array<uint, 3>> meshfaces, float nodeweight) {
    return Ref<SoftBody>(new SoftBody(nodepositions, connections, meshfaces, nodeweight));
}

SoftBody::SoftBody(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> c, std::vector<std::array<uint, 3>> meshfaces, float nodeweight) {
    
    connections = {};


    //Nodes initialization
    worknodes.resize(nodepositions.size());
    for (size_t i = 0; i < nodepositions.size(); i++) {
        worknodes[i].position = nodepositions[i];
        worknodes[i].velocity = {0,0,0};
        worknodes[i].acceleration = {0,0,0};
        worknodes[i].mass = nodeweight;
    }

    nodedata.resize(nodepositions.size(), NodeData());
    for (size_t i = 0; i < nodedata.size(); i++) {
        nodedata[i].node = i;
    }

    connections.resize(c.size());
    for (size_t i = 0; i < c.size(); i++) {
        connections[i].node1 = c[i].first;
        connections[i].node2 = c[i].second;
        connections[i].neutrallen = connections[i].GetLength(worknodes);
        auto& node1 = worknodes[connections[i].node1];
        auto& node2 = worknodes[connections[i].node2];
        node1.connections.push_back(i);
        node2.connections.push_back(i);
    }
    Swap();
}

void SoftBody::Swap() {
    copying.lock();
    drawnodes = worknodes;
    copying.unlock();
}
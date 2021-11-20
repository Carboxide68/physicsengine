#pragma once

#include "Node.h"
#include "connection.h"
#include "Oxide/Scene/Mesh.h"
#include <tracy/Tracy.hpp>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

using namespace Oxide;

struct SoftRepresentation {

    std::vector<Node> Nodes;
    std::vector<Connection> *Connections;

    void Copy(const SoftRepresentation& source) {
        ZoneScoped;
        Nodes = source.Nodes;
        Connections = source.Connections;
    }
};

class SoftBody {
public:

    inline glm::vec3 ForceCalculator(const Connection* c, const std::vector<Node>& nodes) {
        const float lengthdiff = c->GetLength(nodes) - c->neutrallen;
        return K * lengthdiff * glm::normalize(nodes[c->node2].position - nodes[c->node1].position);
    }

    std::atomic<float> K {98.0f};
    std::atomic<float> drag {0.1f};

    static Ref<SoftBody> Create(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> connections, std::vector<std::array<uint, 3>> meshfaces, float weight);

    //Should return a vector in the direction of the second node relative to the first node with a force.
    std::vector<Connection> connections;

    void Swap();

    std::vector<NodeData> nodedata;
    std::vector<Node> worknodes;
    std::vector<Node> drawnodes;

    std::mutex copying;

private:

    SoftBody(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> connections, std::vector<std::array<uint, 3>> meshfaces, float weight);

    std::vector<glm::vec3> global_forces = {};
    std::vector<glm::vec3> global_accelerations {{0, -9.8, 0}};

    friend class PhysicsHandler;

};
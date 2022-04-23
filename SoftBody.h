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
        const glm::vec3 diff = nodes[c->node2].position - nodes[c->node1].position;
        const glm::vec3 adjusted_diff = diff - glm::normalize(diff) * c->neutrallen;
        return K.load() * adjusted_diff;
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

    std::vector<glm::vec3> global_forces = {};
    std::vector<glm::vec3> global_accelerations {{0, -9.8, 0}};
private:

    SoftBody(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> connections, std::vector<std::array<uint, 3>> meshfaces, float weight);


    friend class PhysicsHandler;

};

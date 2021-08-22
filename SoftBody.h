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
    std::vector<Connection> Connections;

    void Copy(const SoftRepresentation& source) {
        Nodes.resize(source.Nodes.size());
        Connections.resize(source.Connections.size());
        const intptr_t nodeArrayOffset = &Nodes[0] - &source.Nodes[0];
        for (size_t i = 0; i < Nodes.size(); i++) {
            Nodes[i] = source.Nodes[i];
        }
        for (size_t i = 0; i < source.Connections.size(); i++) {
            Connections[i] = source.Connections[i];
            Connections[i].node1 += nodeArrayOffset;
            Connections[i].node2 += nodeArrayOffset;
        }
    }

};

class SoftBody {
public:

    SoftRepresentation workbody;
    SoftRepresentation body;
    std::mutex bodyMutex;
    std::mutex workbodyMutex;
    Mesh BoundingBody;

    ~SoftBody() {
        if (copythread.joinable()) {
            copythread.join();
        }
    }

    std::atomic<float> drag = 0.1f;

    static Ref<SoftBody> Create(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> connections, std::vector<std::array<uint, 3>> meshfaces, float weight);
    static glm::vec3 STANDARD_FORCE_CALCULATOR(const Connection*);

    //Should return a vector in the direction of the second node relative to the first node with a force.
    std::function<glm::vec3(const Connection*)> ForceCalculator = STANDARD_FORCE_CALCULATOR;

    void CopyOver();
    void CopyBack();
    void Join();

private:


    SoftBody(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> connections, std::vector<std::array<uint, 3>> meshfaces, float weight);

    static void copyview(const SoftRepresentation& source, SoftRepresentation& target, std::mutex& mtx);
    static void copyback(const SoftRepresentation& source, SoftRepresentation& target, std::mutex& mtx);
    std::thread copythread;

};
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
    static glm::vec3 STANDARD_FORCE_CALCULATOR(const Connection*, const std::vector<Node>& nodes);

    //Should return a vector in the direction of the second node relative to the first node with a force.
    std::function<glm::vec3(const Connection*, const std::vector<Node>&)> ForceCalculator = STANDARD_FORCE_CALCULATOR;

    void CopyOver();
    void CopyBack();

private:


    SoftBody(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> connections, std::vector<std::array<uint, 3>> meshfaces, float weight);

    static void copyview(const SoftRepresentation& source, SoftRepresentation& target, std::mutex& mtx);
    static void copyback(const SoftRepresentation& source, SoftRepresentation& target);
    std::thread copythread;

    std::vector<Connection> m_Connections;

};
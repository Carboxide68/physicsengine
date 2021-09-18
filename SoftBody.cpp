#include "SoftBody.h"
#include "globals.h"

Ref<SoftBody> SoftBody::Create(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> connections, std::vector<std::array<uint, 3>> meshfaces, float nodeweight) {
    return Ref<SoftBody>(new SoftBody(nodepositions, connections, meshfaces, nodeweight));
}

SoftBody::SoftBody(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> c, std::vector<std::array<uint, 3>> meshfaces, float nodeweight) {

    m_Connections = {};

    workbody.Nodes.resize(nodepositions.size(), Node());
    for (int i = 0; i < nodepositions.size(); i++) {
        workbody.Nodes[i].position = nodepositions[i];
        workbody.Nodes[i].mass = nodeweight;
    }
    int i = 0;
    m_Connections.reserve(c.size());
    for (auto& connection : c) {
        Node& node1 = workbody.Nodes[connection.first];
        Node& node2 = workbody.Nodes[connection.second];
        m_Connections.push_back({connection.first, connection.second, glm::length(node1.position - node2.position)});
        node1.connections.push_back(i);
        node2.connections.push_back(i);
        i++;
    }
    workbody.Connections = &m_Connections;
    CopyOver();
    PhysicsThreadPool.wait_for_tasks();
}

glm::vec3 SoftBody::STANDARD_FORCE_CALCULATOR(const Connection* c, const std::vector<Node>& nodes) {
    const float K = 98;
    const float lengthdiff = c->GetLength(nodes) - c->neutrallen;
    return K * lengthdiff * glm::normalize(nodes[c->node2].position - nodes[c->node1].position);
}

void SoftBody::CopyOver() {
    ZoneScoped
    PhysicsThreadPool.push_task(copyview, std::cref(workbody), std::ref(body), std::ref(bodyMutex));
}

void SoftBody::CopyBack() {
    PhysicsThreadPool.push_task(copyback, std::cref(body), std::ref(workbody));
}

void SoftBody::copyview(const SoftRepresentation& source, SoftRepresentation& target, std::mutex& mtx) {
    ZoneScoped;
    SoftRepresentation tmp;
    tmp.Nodes = source.Nodes;
    tmp.Connections = source.Connections;
    mtx.lock();
    target.Nodes = tmp.Nodes;
    target.Connections = tmp.Connections;
    mtx.unlock();
}

void SoftBody::copyback(const SoftRepresentation& source, SoftRepresentation& target) {
    if (source.Nodes.size() == target.Nodes.size()) {
        for (size_t i = 0; i < source.Nodes.size(); i++) {
            target.Nodes[i].islocked.store(source.Nodes[i].islocked.load());
            target.Nodes[i].mass = source.Nodes[i].mass;
            target.Nodes[i].imguiopen.store(source.Nodes[i].imguiopen.load());
            target.Nodes[i].drawconnections.store(source.Nodes[i].drawconnections.load());
        }
    }
}
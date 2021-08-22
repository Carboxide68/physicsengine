#include "SoftBody.h"

Ref<SoftBody> SoftBody::Create(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> connections, std::vector<std::array<uint, 3>> meshfaces, float nodeweight) {
    return Ref<SoftBody>(new SoftBody(nodepositions, connections, meshfaces, nodeweight));
}

SoftBody::SoftBody(std::vector<glm::vec3> nodepositions, std::vector<std::pair<uint, uint>> c, std::vector<std::array<uint, 3>> meshfaces, float nodeweight) {

    workbody.Connections = {};

    workbody.Nodes.resize(nodepositions.size(), Node());
    for (int i = 0; i < nodepositions.size(); i++) {
        workbody.Nodes[i].position = nodepositions[i];
        workbody.Nodes[i].mass = nodeweight;
    }

    int i = 0;
    workbody.Connections.reserve(c.size());
    for (auto& connection : c) {
        Node* node1 = &(workbody.Nodes[connection.first]);
        Node* node2 = &(workbody.Nodes[connection.second]);
        workbody.Connections.push_back({node1, node2, glm::length(node1->position - node2->position)});
        node1->connections.push_back(&workbody.Connections[i]);
        node2->connections.push_back(&workbody.Connections[i]);
        i++;
    }
    CopyOver();
    Join();
}

glm::vec3 SoftBody::STANDARD_FORCE_CALCULATOR(const Connection* c) {
    const float K = 98;
    const float lengthdiff = c->GetLength() - c->neutrallen;
    return K * lengthdiff * glm::normalize(c->node2->position - c->node1->position);
}

void SoftBody::CopyOver() {
    if (copythread.joinable()) {
        copythread.join();
    }
    copythread = std::thread(copyview, std::cref(workbody), std::ref(body), std::ref(workbodyMutex));
}

void SoftBody::CopyBack() {
    if (copythread.joinable()) {
        copythread.join();
    }
    copythread = std::thread(copyback, std::cref(body), std::ref(workbody), std::ref(bodyMutex));
}

void SoftBody::copyview(const SoftRepresentation& source, SoftRepresentation& target, std::mutex& mtx) {
    mtx.lock();
    target.Copy(source);
    mtx.unlock();
}

void SoftBody::copyback(const SoftRepresentation& source, SoftRepresentation& target, std::mutex& mtx) {
    mtx.lock();
    if (source.Nodes.size() == target.Nodes.size()) {
        for (size_t i = 0; i < source.Nodes.size(); i++) {
            target.Nodes[i].islocked = source.Nodes[i].islocked;
            target.Nodes[i].mass = source.Nodes[i].mass;
            target.Nodes[i].imguiopen = source.Nodes[i].imguiopen;
            target.Nodes[i].drawconnections = source.Nodes[i].drawconnections;
        }
    }
    mtx.unlock();

}

void SoftBody::Join() {
    copythread.join();
}
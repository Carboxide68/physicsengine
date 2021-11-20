
#include "PhysicsHandler.h"
#include "Oxide/Scene/Model.h"
#include <imgui/imgui.h>
#include <algorithm>
#include <tracy/Tracy.hpp>
#include <thread>
#include <mutex>
#include "globals.h"

void PhysicsHandler::OnStart() {
    {
    Model tempmodel;
    tempmodel.LoadModel("resource/node.obj");
    m_NodeProp = tempmodel.Meshes[0];
    }

    m_MeshRenderer = std::static_pointer_cast<MeshRenderer>(Game->scene->GetRenderer("MeshRenderer"));
    m_MeshRenderer->Load(m_MeshNodeHandle, m_NodeProp);

    m_GizmoDrawer = std::static_pointer_cast<GizmoDrawer>(Game->scene->GetRenderer("GizmoDrawer"));
    PhysicsThreadPool.sleep_duration = 0;
}

void PhysicsHandler::EachFrame() {

    DrawUI();
    DrawNodes();

    if (m_PhysicsThread.joinable()) {
        if (stop.load()) {
            m_PhysicsThread.join();
        }
    } else {
        if (!stop.load()) {
            m_PhysicsThread = std::thread(EngineMain, PhysicsCtx(m_Softbodies, executing, run, stop, TS, time_passed, average_tick_time));
        }
    }
}

PhysicsHandler::~PhysicsHandler() {
    stop.store(true);
    run.store(0);
    if (m_PhysicsThread.joinable()) {
        m_PhysicsThread.join();
    }
}

void PhysicsHandler::AddSoftBody(Ref<SoftBody> body) {
    executing.lock();
    m_Softbodies.push_back(body);
    executing.unlock();
}

void PhysicsHandler::DrawUI() {

    auto tick_time = average_tick_time.load();

    ImGui::Text("Tick time: %fms | TPS %f", tick_time * 1000.0, 1.0/tick_time);
    ImGui::Text("Time passed: %fs", time_passed.load());
    ImGui::NewLine();
    if (ImGui::Button((stop.load()) ? "Start" : "Stop")) {
        stop.store(!stop.load());
    }
    if (ImGui::Button("Pause/Unpause")) {
        run.store((!run.load()) ? -1 : 0);
    }

    float ts = TS.load();

    ImGui::SliderFloat("Tick Speed", &ts, 0.00001f, 0.1f, "%.7f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SliderFloat("Draw Scale", &m_DrawScale, 0.0001f, 0.1f, "%.5f");

    TS.store(ts);

    for (size_t i = 0; i < m_Softbodies.size(); i++) {
        auto& body = *m_Softbodies[i];
        body.copying.lock();

        char bodyname[20];
        sprintf(bodyname, "Body %lu", i);
        if (ImGui::TreeNode(bodyname)) {

            float drag = body.drag.load();
            ImGui::SliderFloat("Drag", &drag, 0.0f, 1.0f, "%.6f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
            body.drag.store(drag);

            float K = body.K.load();
            ImGui::SliderFloat("K", &K, 1.0f, 100000.0f, "%.6f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
            body.K.store(K);

            for (auto& nodedata : body.nodedata) {
                DrawNodeUI(nodedata, body.drawnodes[nodedata.node]);
            }
            ImGui::TreePop();
        }
        body.copying.unlock();
    }
}

void PhysicsHandler::DrawNodeUI(NodeData& data, const Node& node) {

    if (ImGui::TreeNode(&data.imgui_open, "Node %d", data.node)) {

        ImGui::Text("Position: %f, %f, %f", node.position.x, node.position.y, node.position.z);
        ImGui::Text("Velocity: %f, %f, %f", node.velocity.x, node.velocity.y, node.velocity.z);
        ImGui::Text("Acceleration: %f, %f, %f", node.acceleration.x, node.acceleration.y, node.acceleration.z);
        
        bool locked = data.is_locked.load();
        ImGui::Checkbox("Lock", &locked);
        data.is_locked.store(locked);

        ImGui::Checkbox("Draw Connections", &data.draw_connections);

        ImGui::TreePop();
    }

}

void PhysicsHandler::DrawNodes() {
    glm::mat4 node_model_matrix = glm::mat4(m_DrawScale);
    node_model_matrix[3][3] = 1.f;

    for (auto& body : m_Softbodies) {
        std::vector<uint> drawn_connections = {};

        for (size_t i = 0; i < body->drawnodes.size(); i++) {
            auto& node = body->drawnodes[i];
            auto& nodedata = body->nodedata[i];

            //Set the position of every node
            node_model_matrix[3][0] = node.position.x;
            node_model_matrix[3][1] = node.position.y;
            node_model_matrix[3][2] = node.position.z;

            m_MeshRenderer->DrawInstance(m_MeshNodeHandle, node_model_matrix);

            if (nodedata.draw_connections) {
                drawn_connections.insert(drawn_connections.end(), node.connections.begin(), node.connections.end());
            }
        }

        std::sort(drawn_connections.begin(), drawn_connections.end());

        //Make everything in list unique copies of itself
        {
        size_t head = 0;
        uint last = (uint)-1;
        for (uint con : drawn_connections) {
            if (con != last) {
                drawn_connections[head] = con;
                head++;
                last = con;
            }
        }
        drawn_connections.resize(head);
        }

        for (uint c : drawn_connections) {
            auto& nodeIndex1 = body->connections[c].node1;
            auto& nodeIndex2 = body->connections[c].node2;

            auto& point1 = body->drawnodes[nodeIndex1].position;
            auto& point2 = body->drawnodes[nodeIndex2].position;

            m_GizmoDrawer->DrawLine(point1, point2);
        }
    }

}

#include "physics.inl"
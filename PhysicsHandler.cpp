
#include "PhysicsHandler.h"
#include "Oxide/Scene/Model.h"
#include <imgui/imgui.h>
#include <algorithm>
#include <tracy/Tracy.hpp>
#include <thread>
#include <mutex>

const char _physics_frame[] = "Physics Tick";

void PhysicsHandler::OnStart() {

    {
    Model tempmodel;
    tempmodel.LoadModel("resource/node.obj");
    m_NodeProp = tempmodel.Meshes[0];
    }

    meshrenderer = std::static_pointer_cast<MeshRenderer>(Game->scene->GetRenderer("MeshRenderer"));
    meshrenderer->Load(m_NodeMeshHandel, m_NodeProp);

    gizmodrawer = std::static_pointer_cast<GizmoDrawer>(Game->scene->GetRenderer("GizmoDrawer"));
}

PhysicsHandler::~PhysicsHandler() {
    stop = true;
    physicsthread.join();
}

void PhysicsHandler::EachFrame() {
    DrawBodies();
    DrawUI();

    if (!stop) {
        if (!physicsthread.joinable()) {
            physicsthread = std::thread(DoPhysics, PhysicsCtx(usingBodiesArray, m_Bodies, pause, stop, TS, time_passed));
        }
    } else {
        if (physicsthread.joinable()) {
            physicsthread.join();
        }
    }
}

void PhysicsHandler::DrawUI() {

    ImGui::Text("Time passed: %fs", time_passed.load());
    if (ImGui::Button("Pause/Unpause")) {

        pause = (pause != 0) ? 0 : -1;
    }
    if (ImGui::Button("Single tick")) {
        pause = 1;
    }
    float ts = (float)TS;
    ImGui::InputFloat("Tick Speed", &ts, 0.0f, 0.0f, "%.7f");
    TS = ts;
    for (int i = 0; i < m_Bodies.size(); i++) {
        m_Bodies[i]->bodyMutex.lock();
        if (ImGui::TreeNode(&i, "Body %d", i)) {
            float energy = CalculateEnergy(*m_Bodies[i]);
            ImGui::Text("Energy: %fJ", energy);
            ImGui::SliderFloat("Scale", &m_DrawScales[i], 0.00001f, 1.0f, "%.6f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
            float f = m_Bodies[i]->drag.load();
            ImGui::SliderFloat("Drag", &f, 0.00001f, 1.0f, "%.6f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
            m_Bodies[i]->drag.store(f);
            for (size_t j = 0; j < m_Bodies[i]->body.Nodes.size(); j++) {
                if (ImGui::TreeNode(&m_Bodies[i]->body.Nodes[j].imguiopen, "Node %d", j)) {
                    ImGui::Text("Position: %f, %f, %f", m_Bodies[i]->body.Nodes[j].position.x, m_Bodies[i]->body.Nodes[j].position.y, m_Bodies[i]->body.Nodes[j].position.z);
                    ImGui::Text("Force: %f, %f, %f", m_Bodies[i]->body.Nodes[j].force.x, m_Bodies[i]->body.Nodes[j].force.y, m_Bodies[i]->body.Nodes[j].force.z);
                    ImGui::Text("Velocity: %f, %f, %f", m_Bodies[i]->body.Nodes[j].velocity.x, m_Bodies[i]->body.Nodes[j].velocity.y, m_Bodies[i]->body.Nodes[j].velocity.z);
                    if (ImGui::Checkbox("Draw connections", &m_Bodies[i]->body.Nodes[j].drawconnections)) {
                        m_Bodies[i]->CopyBack();
                    }
                    if (ImGui::Checkbox("Lock", &m_Bodies[i]->body.Nodes[j].islocked)) {
                        m_Bodies[i]->CopyBack();
                    };
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        m_Bodies[i]->bodyMutex.unlock();
    }
}

void PhysicsHandler::DrawBodies() {
    usingBodiesArray.lock();
    for (size_t i = 0; i < m_Bodies.size(); i++) {
        m_Bodies[i]->bodyMutex.lock();
        std::vector<Connection*> drawConnections = {};
        size_t j = 0;
        for (auto& node : m_Bodies[i]->body.Nodes) {
            glm::mat4 tmpmat = glm::mat4(1);
            tmpmat[0][0] = m_DrawScales[i];
            tmpmat[1][1] = m_DrawScales[i];
            tmpmat[2][2] = m_DrawScales[i];

            tmpmat[3][0] = node.position.x;
            tmpmat[3][1] = node.position.y;
            tmpmat[3][2] = node.position.z;
            meshrenderer->DrawInstance(m_NodeMeshHandel, tmpmat);

            if (node.drawconnections) {
                drawConnections.insert(drawConnections.end(), m_Bodies[i]->body.Nodes[j].connections.begin(), m_Bodies[i]->body.Nodes[j].connections.end());
            }
        std::vector<std::vector<char>> m_DrawNodes;
            j++;
        }
        
        std::sort(drawConnections.begin(), drawConnections.end());

        size_t head = 0;
        Connection* last = nullptr;
        for (auto& con : drawConnections) {
            if (con != last) {
                drawConnections[head] = con;
                head++;
                last = con;
            }
        }
        drawConnections.resize(head);

        for (auto& connection : drawConnections) {
            glm::vec3 point1 = connection->node1->position;
            glm::vec3 point2 = connection->node2->position;
            gizmodrawer->DrawLine(point1, point2);
        }
        m_Bodies[i]->bodyMutex.unlock();
    }
    usingBodiesArray.unlock();
}

float PhysicsHandler::CalculateEnergy(const SoftBody& body) const {
    double energy = 0;
    for (auto& node : body.body.Nodes) {
        energy += powf(glm::length(node.velocity), 2.0f) * node.mass/2.0f;
        energy += node.position.z * 9.8 * node.mass;
    }
    return (float)energy;
}

void PhysicsHandler::AddSoftBody(Ref<SoftBody> body) {
    usingBodiesArray.lock();
    m_Bodies.push_back(body);
    usingBodiesArray.unlock();
    m_DrawScales.push_back(0.01);
    m_Collapsed.push_back(true);
}

void PhysicsHandler::CalculateForces(const SoftBody& body, SoftRepresentation& source) {
    ZoneScoped;
    const float d = body.drag.load();

    for (auto& node : source.Nodes) {
        //Apply a force that's in the opposite direction of the velocity
        node.force = node.velocity * d * -1.0f;
        node.force += glm::vec3(0.0f, -9.8f * node.mass, 0.0f);
    }
    
    //Get forces from nodes
    {
    ZoneScopedN("Connection calculations");
    for (auto& c : source.Connections) {
        glm::vec3 force = body.ForceCalculator(&c);
        c.node1->force += force;
        c.node2->force += -1.0f * force;
    }
    }
    //Breaking force, speed-dependant
}

void PhysicsHandler::TickUpdate(SoftBody& body, const double ts) {
    ZoneScoped;
    //Copy over the data
    {
    ZoneScopedN("Update velocity");
    for (auto& node : body.workbody.Nodes) {
        if (node.islocked) continue;
        node.position += node.velocity * (float)ts;
        node.velocity += (((float)ts * node.force)/node.mass);
    }
    }
    //Clear forces of all nodes
    CalculateForces(body, body.workbody);
}

void PhysicsHandler::DoPhysics(PhysicsCtx ctx) {
    while (!ctx.stop) {
        double TS = ctx.TS;
        if (ctx.pause == 0) {std::this_thread::sleep_for(std::chrono::milliseconds(7)); continue;}
        if (ctx.pause > 0) ctx.pause--;
        FrameMarkStart(_physics_frame);
        for (auto body : ctx.Bodies) {
            body->workbodyMutex.lock();
            ZoneScopedN("Single body physics");
            std::vector<glm::vec3> combinedVelocities(body->workbody.Nodes.size());
            std::vector<glm::vec3> firstPositions(body->workbody.Nodes.size());
            
            for (size_t i = 0; i < body->workbody.Nodes.size(); i++) {
                firstPositions[i] = body->workbody.Nodes[i].position;
                combinedVelocities[i] = body->workbody.Nodes[i].velocity;
            }
            TickUpdate(*body, TS/2.0);
            for (size_t i = 0; i < body->workbody.Nodes.size(); i++) {
                combinedVelocities[i] += 2.0f * body->workbody.Nodes[i].velocity;
            }
            TickUpdate(*body, TS/2.0);
            for (size_t i = 0; i < body->workbody.Nodes.size(); i++) {
                combinedVelocities[i] += 2.0f * body->workbody.Nodes[i].velocity;
            }
            TickUpdate(*body, TS);
            for (size_t i = 0; i < body->workbody.Nodes.size(); i++) {
                body->workbody.Nodes[i].position = firstPositions[i] + TSf/6.0f * (combinedVelocities[i] + body->workbody.Nodes[i].velocity);
            }
            body->workbodyMutex.unlock();
            body->CopyOver();
        }
        FrameMarkEnd(_physics_frame);
        ctx.time_passed += TS;
        for (auto body : ctx.Bodies) {
            body->Join();
        }
    }
}
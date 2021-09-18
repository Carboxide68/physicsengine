
#include "PhysicsHandler.h"
#include "Oxide/Scene/Model.h"
#include <imgui/imgui.h>
#include <algorithm>
#include <tracy/Tracy.hpp>
#include <thread>
#include <mutex>
#include "globals.h"

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
    PhysicsThreadPool.sleep_duration = 0;
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
            physicsthread = std::thread(DoPhysics, PhysicsCtx(usingBodiesArray, m_Bodies, pause, stop, TS, time_passed, average_tick_time));
        }
    } else {
        if (physicsthread.joinable()) {
            physicsthread.join();
        }
    }
}

void PhysicsHandler::DrawUI() {
    auto tt = average_tick_time.load();
    ImGui::Text("Tick time: %fms | TPS: %f", tt * 1000.0, 1.0/tt);
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
                Node& node = m_Bodies[i]->body.Nodes[j];
                if (ImGui::TreeNode(&node.imguiopen, "Node %d", j)) {
                    ImGui::Text("Position: %f, %f, %f", node.position.x, node.position.y, node.position.z);
                    ImGui::Text("Force: %f, %f, %f", node.force.x, node.force.y, node.force.z);
                    ImGui::Text("Velocity: %f, %f, %f", node.velocity.x, node.velocity.y, node.velocity.z);
                    ImGui::Text("Connection count: %d", node.connections.size());
                    if(ImGui::Button("Print connections")) {
                        printf("____________________________\n");
                        for (auto& c : node.connections) {
                            printf("[%*d, %*d]\n", 5, (*m_Bodies[i]->body.Connections)[c].node1, 5, (*m_Bodies[i]->body.Connections)[c].node2);
                        }
                    }
                    bool tmpbool = node.drawconnections.load();
                    if (ImGui::Checkbox("Draw connections", &tmpbool)) {
                        node.drawconnections.store(tmpbool);
                    }
                    tmpbool = node.islocked.load();
                    if (ImGui::Checkbox("Lock", &tmpbool)) {
                        node.islocked.store(tmpbool);
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
        auto& b = *m_Bodies[i];
        b.bodyMutex.lock();
        std::vector<uint> drawConnections = {};
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
        uint last = (uint)-1;
        for (auto& con : drawConnections) {
            if (con != last) {
                drawConnections[head] = con;
                head++;
                last = con;
            }
        }
        drawConnections.resize(head);

        for (uint& c : drawConnections) {
            glm::vec3 point1 = b.body.Nodes[(*b.body.Connections)[c].node1].position;
            glm::vec3 point2 = b.body.Nodes[(*b.body.Connections)[c].node2].position;
            gizmodrawer->DrawLine(point1, point2);
        }
        m_Bodies[i]->bodyMutex.unlock();
    }
    usingBodiesArray.unlock();
}

float PhysicsHandler::CalculateEnergy(const SoftBody& body) const {
    ZoneScoped
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

void PhysicsHandler::CalculateForces(std::function<glm::vec3(const Connection*, const std::vector<Node>&)> forceFunction, SoftRepresentation& source, const float d) {
    ZoneScoped;
    for (auto& node : source.Nodes) {
        //Apply a force that's in the opposite direction of the velocity
        node.force = node.velocity * d * -1.0f;
        node.force += glm::vec3(0.0f, -9.8f * node.mass, 0.0f);
    }
    
    //Get forces from nodes
    {
    ZoneScopedN("Connection calculations");
    PhysicsThreadPool.parallelize_loop(0, (*source.Connections).size(), [&forceFunction, &source](uint start, uint end) -> void {
        ZoneScopedN("loopchunk");
        for (uint i = start; i < end; i++) {
            auto& c = (*source.Connections)[i];
            glm::vec3 force = forceFunction(&c, source.Nodes);
            source.Nodes[c.node1].force += force;
            source.Nodes[c.node2].force += -1.0f * force;
        }
    }, PhysicsThreadPool.get_thread_count() - 2);
    }
}

void PhysicsHandler::DoTimeStep(const SoftRepresentation& repr, const SoftRepresentation& derivative, SoftRepresentation& out, const double ts) {
    ZoneScoped;
    for (size_t i = 0; i < repr.Nodes.size(); i++) {
        auto& rNode = repr.Nodes[i];
        auto& dNode = derivative.Nodes[i];
        auto& oNode = out.Nodes[i];
        oNode.islocked.store(rNode.islocked.load());
        if (rNode.islocked) continue;
        oNode.position = rNode.position + dNode.velocity * (float)ts;
        oNode.velocity = rNode.velocity + (((float)ts * dNode.force)/rNode.mass);
    }
}

void PhysicsHandler::DoPhysics(PhysicsCtx ctx) {
    SoftRepresentation tmp;
    std::deque<double> tick_times(30, 0.0);

    using clock = std::chrono::high_resolution_clock;

    while (!ctx.stop) {
        double TS = ctx.TS;
        if (ctx.pause == 0) {std::this_thread::sleep_for(std::chrono::milliseconds(7)); continue;}
        if (ctx.pause > 0) ctx.pause--;
        FrameMarkStart(_physics_frame);
        auto tick_time = clock::now();
        for (auto body : ctx.Bodies) {
            ZoneScopedN("Single body physics");
            std::vector<glm::vec3> combinedVelocities(body->workbody.Nodes.size());
            std::vector<glm::vec3> combinedForces(body->workbody.Nodes.size());
            std::vector<glm::vec3> firstPositions(body->workbody.Nodes.size());
            std::vector<glm::vec3> firstVelocities(body->workbody.Nodes.size());

            const float d = body->drag.load();
            tmp.Nodes.resize(body->workbody.Nodes.size());
            tmp.Connections = body->workbody.Connections;

            //K1
            for (size_t i = 0; i < body->workbody.Nodes.size(); i++) {
                firstPositions[i] = body->workbody.Nodes[i].position;
                firstVelocities[i] = body->workbody.Nodes[i].velocity;
                combinedVelocities[i] = body->workbody.Nodes[i].velocity;
                combinedForces[i] = body->workbody.Nodes[i].force;
                tmp.Nodes[i].position = body->workbody.Nodes[i].position;
                tmp.Nodes[i].velocity = body->workbody.Nodes[i].velocity;
                tmp.Nodes[i].force = body->workbody.Nodes[i].force;
                tmp.Nodes[i].islocked.store(body->workbody.Nodes[i].islocked);
            }
            DoTimeStep(body->workbody, tmp, tmp, TS/2.0);

            //k2
            CalculateForces(body->ForceCalculator, tmp, d);
            for (size_t i = 0; i < tmp.Nodes.size(); i++) {
                combinedVelocities[i] += 2.0f * body->workbody.Nodes[i].velocity;
                combinedForces[i] += 2.0f * body->workbody.Nodes[i].force;
            }
            DoTimeStep(body->workbody, tmp, tmp, TS/2.0);

            //K3
            CalculateForces(body->ForceCalculator, tmp, d);
            for (size_t i = 0; i < tmp.Nodes.size(); i++) {
                combinedVelocities[i] += 2.0f * body->workbody.Nodes[i].velocity;
                combinedForces[i] += 2.0f * body->workbody.Nodes[i].force;
            }
            DoTimeStep(body->workbody, tmp, tmp, TS);

            //K4
            CalculateForces(body->ForceCalculator, tmp, d);
            for (size_t i = 0; i < tmp.Nodes.size(); i++) {
                if (body->workbody.Nodes[i].islocked) continue;
                body->workbody.Nodes[i].position = firstPositions[i] + TSf/6.0f * (combinedVelocities[i] + tmp.Nodes[i].velocity);
                body->workbody.Nodes[i].velocity = firstVelocities[i] + TSf/6.0f * (combinedForces[i] + tmp.Nodes[i].force);
            }
            CalculateForces(body->ForceCalculator, body->workbody, d);
            body->CopyOver();
        }
        FrameMarkEnd(_physics_frame);
        ctx.time_passed += TS;

        //Fixing tick time
        tick_times.pop_back();
        double tdiff = std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch() - tick_time.time_since_epoch()).count();
        tick_times.push_front(tdiff/(1000.0 * 1000.0));
        double tottime = 0.0;
        for (auto& time : tick_times) {
            tottime += time;
        }
        tottime /= 30.0;
        ctx.tick_time.store(tottime);
    }
    PhysicsThreadPool.wait_for_tasks();
}
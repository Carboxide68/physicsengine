#include <iostream>
#include <vector>
#include <cmath>

#include <Oxide.h>
#include "Oxide/Resource/DebugCamera.h"
#include "PhysicsHandler.h"
#include "Drawer.h"
#include <glm/glm.hpp>
#include <imgui/imgui.h>

int ITERATIONS = 10000;
int invocations;

size_t node_count = 1000;
float size = 1.f;
float TS = 0.001f;
float drag = 0.05f;
float K = 98.f;

float mass = 0.2f;

int buffer_state = -1;
bool just_load = true;
bool use_gravity = true;

size_t nodes_size;
size_t connections_size;

size_t mesh_size = 0;

Ref<PhysicsHandler> ph;

void buildPendulum(std::vector<std::pair<uint, uint>>& connections, std::vector<glm::vec3>& nodes);
void buildSheet(std::vector<std::pair<uint, uint>>& connections, std::vector<glm::vec3>& nodes);
void buildCube(std::vector<std::pair<uint, uint>>& connections, std::vector<glm::vec3>& nodes);

void benchmarkPendulum();
void benchmarkSheet();
void benchmarkCube();

void test();

using namespace Oxide;

class WindowHandler : public Actor {
public:

    void OnStart() override {

        PerspectiveCamera* camera = (PerspectiveCamera*)Game->scene->camera.get();
        Game->window.renderer->SetClearColor(1, 1, 1, 1);
        Game->crenderer->Disable(GL_CULL_FACE);
        Game->crenderer->Enable(GL_DEPTH_TEST);
        camera->SetFOV(60.0f);

        WindowSettings settings = {"Testing", 720, 500, true};
        Game->window.UpdateSettings(settings);

        Game->window.renderer->SetViewport(2560, 1440);
        camera->SetAspect(2560.0/1440.0);
        camera->SetPosition({2, 0, 2});
        camera->LookAt({0, 0, 0});
    }

    void EachFrame() override {
        glm::vec3 position = Game->scene->camera->GetPosition();
        glm::vec3 lookingdir = Game->scene->camera->GetLookAt();
        ImGui::Text("Looking dir: %f, %f, %f", lookingdir.x, lookingdir.y, lookingdir.z);
        ImGui::Text("Position: %f, %f, %f", position.x, position.y, position.z);

        ImGui::Begin("Settings");
        ImGui::InputInt("Iterations", &ITERATIONS);
        {
        int nc = node_count;
        if (ImGui::InputInt("Node count", &nc)) {
            node_count = nc;
        }
        }
        ImGui::InputFloat("Mass", &mass);
        ImGui::InputFloat("K", &K);
        ImGui::InputFloat("Drag", &drag);
        ImGui::Checkbox("Gravity", &use_gravity);

        if (ImGui::Button("Pendulum Benchmark")) {
            benchmarkPendulum();
        }
        if (ImGui::Button("Sheet Benchmark")) {
            benchmarkSheet();
        }
        if (ImGui::Button("Cube Benchmark")) {
            benchmarkCube();
        }
        if (ImGui::Button("Test")) {
            test();
        }
        ImGui::Checkbox("Just Load", &just_load);
        ImGui::End();
    }

private:

ACTOR_ESSENTIALS(WindowHandler);


};

int main(int argc, char *argv[]) {

    Game->Init();

    Game->scene->AddRenderer<MeshRenderer>();
    Game->scene->AddRenderer<GizmoDrawer>();

    Game->scene->AddActor<WindowHandler>();
     ph = Game->scene->AddActor<PhysicsHandler>();
    Game->scene->AddActor<DebugCamera>();

    Game->Start();

}

void benchmarkPendulum() {
    printf("_______________________________________\nTest Pendulum:\n");

    std::vector<std::pair<uint, uint>> connections;
    std::vector<glm::vec3> nodes;
    Ref<SoftBody> sb;
    const float mass_fraction = mass/(float)node_count;
    {
    ZoneScopedN("Build");
    buildPendulum(connections, nodes);
    
    sb = SoftBody::Create(nodes, connections, {}, mass_fraction);

    sb->nodedata[0].is_locked.store(true);
    sb->K.store(K);
    sb->drag.store(drag);
    ph->ClearSoftBodies();
    ph->AddSoftBody(sb);
    }
    printf("Node Count: %lu\nConnection Count: %lu\n", nodes.size(), connections.size());

    if (just_load) return;
    std::array<float, 3> energies_before = ph->CalculateEnergies();

    ph->DoTicks(ITERATIONS);
    ph->stop.store(false);
}


void benchmarkSheet() {

    printf("_______________________________________\nTest Sheet:\n");
    ph->ClearSoftBodies();

    std::vector<std::pair<uint, uint>> connections;
    std::vector<glm::vec3> nodes;
    Ref<SoftBody> sb;
    const float mass_fraction = mass/(float)node_count;
    {
    ZoneScopedN("Build");
    buildSheet(connections, nodes);
    
    sb = SoftBody::Create(nodes, connections, {}, mass_fraction);
    const uint node_count_1D = sqrt(node_count);
    sb->nodedata[0].is_locked.store(true);
    sb->nodedata[node_count_1D-1].is_locked.store(true);
    sb->nodedata[(node_count_1D - 1) * node_count_1D].is_locked.store(true);
    sb->nodedata[node_count_1D * node_count_1D - 1].is_locked.store(true);
    sb->K.store(K);
    sb->drag.store(drag);
    ph->ClearSoftBodies();
    ph->AddSoftBody(sb);
    }
    printf("Node Count: %lu\nConnection Count: %lu\n", nodes.size(), connections.size());

    if (just_load) return;
    std::array<float, 3> energies_before = ph->CalculateEnergies();

    ph->DoTicks(ITERATIONS);
    ph->stop.store(false);

}

std::array<float, 3> energies_before {0,0,0};
void benchmarkCube() {
    printf("_______________________________________\nTest Cube:\n");

    std::vector<std::pair<uint, uint>> connections;
    std::vector<glm::vec3> nodes;
    Ref<SoftBody> sb;
    const float mass_fraction = mass/(float)node_count;
    {
    ZoneScopedN("Build");
    buildCube(connections, nodes);
    
    sb = SoftBody::Create(nodes, connections, {}, mass_fraction);
    const size_t node_count_1D = glm::pow((double)node_count, 1.0/3.0);
    for (size_t x = 0; x < node_count_1D; x++) {
        for (size_t z = 0; z < node_count_1D; z++) {
            const size_t index = (x) * node_count_1D * node_count_1D +\
                                 (0) * node_count_1D + z;
            sb->nodedata[index].is_locked.store(true); 
        }
    }
    sb->K.store(K);
    sb->drag.store(drag);
    ph->ClearSoftBodies();
    ph->AddSoftBody(sb);
    }
    printf("Node Count: %lu\nConnection Count: %lu\n", nodes.size(), connections.size());

    if (just_load) return;
    energies_before = ph->CalculateEnergies();

    ph->DoTicks(ITERATIONS);
    ph->stop.store(false);
}

void test() {
    auto sb = ph->GetSoftBody(0);
    if (sb.get() == NULL) return;
    for (auto& node : sb->worknodes) {
        
        if (
            std::isnan(  node.mass       ) ||
            std::isnan(  node.position.x ) ||
            std::isnan(  node.position.y ) ||
            std::isnan(  node.position.z ) ||
            std::isnan(  node.velocity.x ) ||
            std::isnan(  node.velocity.y ) ||
            std::isnan(  node.velocity.z ) 
            ) {
        
                printf("Test was invalid! Value found to be NaN!\n");
        }
    }

    std::array<float, 3> energies_after;
    energies_after = ph->CalculateEnergies();

    std::array<float, 4> energies_diff;
    energies_diff[0] = energies_after[0] - energies_before[0];
    energies_diff[1] = energies_after[1] - energies_before[1];
    energies_diff[2] = energies_after[2] - energies_before[2];
    energies_diff[3] = energies_diff[0] + energies_diff[1] + energies_diff[2];

    printf("Energies difference:\n\tKinetic: %f\n\tSpring: %f\n\tGravitational: %f\n\tTotal: %f\n",
            energies_diff[0],
            energies_diff[1],
            energies_diff[2],
            energies_diff[3]);

}

void buildPendulum(std::vector<std::pair<uint, uint>>& connections, std::vector<glm::vec3>& nodes) {

    ZoneScopedS(5);
    static const float angle = PI/4.0;
    nodes.clear();
    nodes.reserve(node_count);
    const float fraction = size/(float)node_count;
    const glm::vec3 d = glm::vec3(glm::cos(angle), -glm::sin(angle), 0);
    const glm::vec3 direction = d * fraction;
    for (size_t i = 0; i < node_count; i++) {
        nodes.push_back(direction * (float)i);
    }
    connections.clear();
    connections.reserve(node_count - 1);
    for (size_t i = 0; i < node_count - 1; i++) {
        connections.emplace_back(i+0, i+1);
    }
}

void buildSheet(std::vector<std::pair<uint, uint>>& connections, std::vector<glm::vec3>& nodes) {

    ZoneScopedS(5);
    const size_t node_count_1D = glm::sqrt(node_count);
    const size_t nc = node_count_1D * node_count_1D;

    nodes.clear();
    nodes.reserve(nc);
    const float fraction = size/(float)node_count_1D;
    for (size_t x = 0; x < node_count_1D; x++) {
        for (size_t z = 0; z < node_count_1D; z++) {
            nodes.emplace_back(x * fraction, 0, z * fraction);
        }
    }
    
    connections.clear();
    size_t counter = 0;
    for (size_t x = 1; x < node_count_1D - 1; x++) {
        for (size_t z = 1; z < node_count_1D - 1; z++) {
            const size_t index = (x) * node_count_1D + (z);
            connections.emplace_back(index, index + 1);
            connections.emplace_back(index, index + node_count_1D - 1); //Diag
            connections.emplace_back(index, index + node_count_1D + 1); //Diag
            connections.emplace_back(index, index + node_count_1D); //Front
        }
        counter++;
    }
    //Edge cases

    for (size_t i = 0; i < node_count_1D - 1; i++) {
        size_t index = i * node_count_1D;
        connections.emplace_back(index, index+1);
        connections.emplace_back(index, index + node_count_1D + 1);
        connections.emplace_back(index, index + node_count_1D);

        index = (i+1) * node_count_1D - 1;
        connections.emplace_back(index, index + node_count_1D);
        connections.emplace_back(index, index + node_count_1D-1);
    }
    for (size_t i = 1; i < node_count_1D - 1; i++) {
        size_t index = i;
        connections.emplace_back(index, index+1);
        connections.emplace_back(index, index + node_count_1D + 1);
        connections.emplace_back(index, index + node_count_1D - 1);
        connections.emplace_back(index, index + node_count_1D);

        index = i + node_count_1D * (node_count_1D-1);
        connections.emplace_back(index, index+1);
    }

    //Edge edge cases
    connections.emplace_back(node_count_1D * (node_count_1D - 1), node_count_1D * (node_count_1D - 1) + 1);
    connections.emplace_back(node_count_1D - 1, node_count_1D * 2 - 1);
    connections.emplace_back(node_count_1D - 1, node_count_1D * 2 - 2);
}

#include "cube_generation.inl"
void buildCube(std::vector<std::pair<uint, uint>>& connections, std::vector<glm::vec3>& nodes) {
    const size_t node_count_1D = glm::pow((double)node_count, 1.0/3.0);
    GenerateNodes(nodes, connections, {node_count_1D, size});
}

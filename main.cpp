
#include "Oxide.h"
#include "Oxide/Resource/DebugCamera.h"
#include "PhysicsHandler.h"
#include "Drawer.h"
#include <imgui/imgui.h>

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
    }

private:

ACTOR_ESSENTIALS(WindowHandler)


};

std::array<std::pair<int, int>, 26> GetAdjacents(int x, int y, int z, uint numpoints) {
    int xmag = numpoints * numpoints;
    int ymag = numpoints;
    int thisnode = x * xmag + y * ymag + z;
    std::array<std::pair<int, int>, 26> perms;
    uint head = 0;
    for (int xd = -1; xd < 2; xd++) {
        for (int yd = -1; yd < 2; yd++) {
            for (int zd = -1; zd < 2; zd++) {
                if (!(xd | yd | zd)) continue;
                if ((xd+x) < 0 || (yd+y) < 0 || (zd+z) < 0) {perms[head] = {-1, -1};}
                else if ((xd+x) >= numpoints || (yd+y) >= numpoints || (zd+z) >= numpoints) {perms[head] = {-1, -1};}
                else {perms[head] = {thisnode, (x+xd) * xmag + (y+yd) * ymag + (z+zd)};}
                head++;
            }
        }
    }
    return perms;
}

 int main(int argc, char**argv) {

    Oxide::Log::Init();
    Oxide::Log::GetCoreLogger()->set_level(spdlog::level::trace);
    Oxide::Log::GetClientLogger()->set_level(spdlog::level::trace);

    Game->Init();

    Game->scene->AddRenderer<MeshRenderer>();
    Game->scene->AddRenderer<GizmoDrawer>();

    Game->scene->AddActor<WindowHandler>();
    Ref<PhysicsHandler> ph = Game->scene->AddActor<PhysicsHandler>();
    Game->scene->AddActor<DebugCamera>();

    std::vector<std::pair<uint, uint>> connections;
    std::vector<std::pair<int, int>> bloatedConnections;
    float scale = 1.0f;
    uint numpoints = 25;
    std::vector<glm::vec3> points(pow(numpoints,3));
    for (int x = 0; x < numpoints; x++) {
        for (int y = 0; y < numpoints; y++) {
            for (int z = 0; z < numpoints; z++) {
                points[x * numpoints * numpoints + y * numpoints + z] = glm::vec3(scale/numpoints * x, scale/numpoints * y, scale/numpoints * z);
                std::array<std::pair<int, int>, 26> perms = GetAdjacents(x, y, z, numpoints);
                bloatedConnections.insert(bloatedConnections.end(), perms.begin(), perms.end());
            }
        }
    }
    for (size_t i = 0; i < bloatedConnections.size(); i++) {
        auto& connection = bloatedConnections[i];
        if (connection.first < 0 || connection.second < 0 || connection.first >= pow(numpoints, 3) || connection.second >= pow(numpoints, 3)) continue;
        bool bad = false;
        for (auto& c : connections) {
            if (c.first == connection.second && c.second == connection.first) {bad = true; break;};
        }
        if (bad) continue;
        connections.push_back(connection);
    }

    Ref<SoftBody> myBody = SoftBody::Create(points, connections, {}, 0.5/(float)(powf(numpoints, 3.0)));
    for (int x = 0; x < numpoints; x++) {
        for (int z = 0; z < numpoints; z++) {
            myBody->workbody.Nodes[x * numpoints * numpoints + numpoints * (0) + z].islocked = true;
        }
    }
    ph->AddSoftBody(myBody);
    // myBody->Nodes[38].position = glm::vec3(10, 0, 0);

    Game->Start();
}
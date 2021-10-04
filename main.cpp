
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

void GenerateConnections(uint numpoints, std::vector<std::pair<uint, uint>>& connections) {
    //2D Sides
    //XZ
    for (int zstep = -1; zstep < 2; zstep+=2) {
        for (int xstep = -1; xstep < 2; xstep+=2) {
            // printf("[Step Combination] (%*d, %*d, %*d) [XZ]\n", 2, xstep, 2, 0, 2, zstep);
            for (int xs = 0; xs < numpoints; xs++) {
                for (int ys = 0; ys < numpoints; ys++) {\
                    if (((xs == 0 || xs == numpoints-1)) && zstep < 0) continue; //Make sure the corners aren't done multiple times
                    int ystep = 0;
                    int x = xs;
                    int y = ys;
                    int z = (zstep<0) ? (numpoints-1) : 0;
                    while (true) {
                        if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                        if (x + xstep >= numpoints || y + ystep >= numpoints || z + zstep >= numpoints) break;
                        connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x+xstep) * numpoints * numpoints + (y+ystep) * numpoints + (z+zstep)});
                        x += xstep;
                        y += ystep;
                        z += zstep;
                    }
                }
            }
        }
    }
    //XY
    for (int ystep = -1; ystep < 2; ystep+=2) {
        for (int xstep = -1; xstep < 2; xstep+=2) {
            for (int xs = 0; xs < numpoints; xs++) {
                for (int zs = 0; zs < numpoints; zs++) {
                    if (((xs == 0 || xs == numpoints-1)) && ystep < 0) continue; //Make sure the corners aren't done multiple times
                    int zstep = 0;
                    int x = xs;
                    int y = (ystep<0) ? (numpoints-1) : 0;
                    int z = zs;
                    while (true) {
                        if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                        if (x + xstep >= numpoints || y + ystep >= numpoints || z + zstep >= numpoints) break;
                        connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x+xstep) * numpoints * numpoints + (y+ystep) * numpoints + (z+zstep)});
                        x += xstep;
                        y += ystep;
                        z += zstep;
                    }
                }
            }
        }
    }
    //ZY
    for (int ystep = -1; ystep < 2; ystep+=2) {
        for (int zstep = -1; zstep < 2; zstep+=2) {
            for (int xs = 0; xs < numpoints; xs++) {
                for (int zs = 0; zs < numpoints; zs++) {
                    if (((zs == 0 || zs == numpoints-1)) && ystep < 0) continue; //Make sure the corners aren't done multiple times
                    int xstep = 0;
                    int x = xs;
                    int y = (ystep<0) ? (numpoints-1) : 0;
                    int z = zs;
                    while (true) {
                        if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                        if (x + xstep >= numpoints || y + ystep >= numpoints || z + zstep >= numpoints) break;
                        connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x+xstep) * numpoints * numpoints + (y+ystep) * numpoints + (z+zstep)});
                        x += xstep;
                        y += ystep;
                        z += zstep;
                    }
                }
            }
        }
    }
    //Diagonals
    for (int ystep = -1; ystep < 2; ystep+=2) {
        for (int xstep = -1; xstep < 2; xstep+=2) {
            for (int zstep = -1; zstep < 2; zstep+=2) {
                for (int xs = 0; xs < numpoints; xs++) {
                    for (int zs = 0; zs < numpoints; zs++) {
                        if ((xs == 0 || xs == numpoints-1) && (zs == 0 || zs == numpoints-1) && ystep<0) continue; //Make sure the corners aren't done multiple times
                        int x = xs;
                        int z = zs;
                        int y = (ystep < 0) ? (numpoints - 1) : 0;
                        while (true) {
                            if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                            if (x + xstep >= numpoints || y + ystep >= numpoints || z + zstep >= numpoints) break;
                            connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x+xstep) * numpoints * numpoints + (y+ystep) * numpoints + (z+zstep)});
                            x += xstep;
                            y += ystep;
                            z += zstep;
                        }
                    }
                }
            }
        }
    }
    //Extra diagonals
    for (int ystep = -1; ystep < 2; ystep+=2) {
        for (int xstep = -1; xstep < 2; xstep+=2) {
            for (int zstep = -1; zstep < 2; zstep+=2) {
                for (int ys = 1; ys < numpoints-1; ys++) {
                    for (int xs = 0; xs < numpoints; xs++) {
                        int ystepsuntil = (ystep<0) ? ys : numpoints - ys - 1; //Make sure that it isn't covered by the previous diagonals, as to not have duplicates
                        int xstepsuntil = (xstep<0) ? xs : numpoints - xs - 1;
                        if (!(xstepsuntil<ystepsuntil)) continue;
                        int x = xs;
                        int z = (zstep < 0) ? numpoints-1 : 0;
                        int y = ys;
                        while (true) {
                            if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                            if (x + xstep >= numpoints || y + ystep >= numpoints || z + zstep >= numpoints) break;
                            connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x+xstep) * numpoints * numpoints + (y+ystep) * numpoints + (z+zstep)});
                            x += xstep;
                            y += ystep;
                            z += zstep;
                        }
                    }
                }
            }
        }
    }
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
    float scale = 1.0f;
    float weight = 0.1f;
    uint numpoints = 5;
    if (argc == 3) {
        numpoints = std::stoi(argv[1]);
        weight = std::stof(argv[2]);
    }
    std::vector<glm::vec3> points(pow(numpoints,3));
    for (int x = 0; x < numpoints; x++) {
        for (int y = 0; y < numpoints; y++) {
            for (int z = 0; z < numpoints; z++) {
                points[x * numpoints * numpoints + y * numpoints + z] = glm::vec3(scale/numpoints * x, scale/numpoints * y, scale/numpoints * z);
                if (x < numpoints - 1)
                    connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x + 1) * numpoints * numpoints + y * numpoints + z});
                if (y < numpoints - 1)
                    connections.push_back({x * numpoints * numpoints + y * numpoints + z, x * numpoints * numpoints + (y+1) * numpoints + z});
                if (z < numpoints - 1)
                    connections.push_back({x * numpoints * numpoints + y * numpoints + z, x * numpoints * numpoints + y * numpoints + z+1});
            }
        }
    }
    GenerateConnections(numpoints, connections);
    Ref<SoftBody> myBody = SoftBody::Create(points, connections, {}, weight);
    for (int x = 0; x < numpoints; x++) {
        for (int z = 0; z < numpoints; z++) {
            myBody->workbody.Nodes[x * numpoints * numpoints + numpoints * (0) + z].islocked.store(true);
        }
    }
    ph->AddSoftBody(myBody);
    // myBody->Nodes[38].position = glm::vec3(10, 0, 0);
    Game->Start();
}

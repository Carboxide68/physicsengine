#include "body.h"
#include "Oxide.h"

int main(int argc, char* argv[]) {

    Oxide::Log::Init();
    Oxide::Log::GetCoreLogger()->set_level(spdlog::level::info);
    Oxide::Log::GetClientLogger()->set_level(spdlog::level::info);

    Body firstBody({}, {2.0f, 3.0f, 7.0f}, glm::vec3(0));
    Body secondBody({}, {5.f, 2.f, 1.f},{2.73646f, 2.3485f, -0.600095f});
    secondBody.rotation = glm::rotate(secondBody.rotation, -0.6f, {0.f, 0.f, 1.f});
    bool itDid = firstBody.Intersects(secondBody);
    printf("The intersection algorithm finished with: %d\n", itDid);
    Oxide::Window myWindow({"Physics engine", 720, 500, true});
    myWindow.renderer->SetClearColor(1,1,1,1);
    myWindow.renderer->ChangeState(Oxide::CRenderer::RenderSettings::FACE_CULLING, false);
    glEnable(GL_DEPTH_TEST);
}
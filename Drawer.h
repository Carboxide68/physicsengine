#pragma once

#include "Oxide.h"
#include "Oxide/Scene/ORenderer.h"

using namespace Oxide;

class GizmoDrawer : public ORenderer {
public:

    std::vector<std::pair<int, std::function<void (void)>>> Queue() override;

    void DrawLine(const glm::vec3& point1, const glm::vec3& point2);

    const std::string TypeName = "GizmoDrawer";

    glm::vec3 LineColor;

private:

    std::vector<std::array<glm::vec3, 2>> m_LineDraws;
    GizmoDrawer(Scene* scene);

    Ref<Buffer> m_LineDrawBuffer;
    Ref<Shader> m_LineShader;
    
    Ref<VertexArray> m_VAO;

    friend Scene;

};
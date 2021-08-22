#include "Drawer.h"

GizmoDrawer::GizmoDrawer(Scene* scene) : ORenderer(scene) {

    m_LineDrawBuffer = Buffer::Create(GL_ARRAY_BUFFER, GL_STREAM_DRAW);
    m_LineDrawBuffer->Alloc(100008);
    LineColor = glm::vec3(0, 0, 0);

    m_LineShader = Shader::Create("LineShader.OS");

    m_VAO = CreateRef<VertexArray>();
    m_VAO->Bind();
    m_LineDrawBuffer->Bind();
    m_VAO->Unbind();

}

void GizmoDrawer::DrawLine(const glm::vec3& point1, const glm::vec3& point2) {
    if (m_LineDraws.size() > 100008/24) return;
    m_LineDraws.push_back({point1, point2});
}

std::vector<std::pair<int, std::function<void (void)>>> GizmoDrawer::Queue() {

    return {{2, [&](void) -> void {
        PerspectiveCamera* camera = (PerspectiveCamera*)(Game->scene->camera.get());
        size_t drawsize = m_LineDraws.size() * 6 * sizeof(float); 
        if (!drawsize) return;
        drawsize = (drawsize < 100008) ? drawsize : 100008;

        m_LineDrawBuffer->BufferData(drawsize, &m_LineDraws[0]);
        m_LineDrawBuffer->Bind();
        m_VAO->Bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);
        glm::mat4 matrix = camera->GetPerspectiveMatrix() * camera->GetViewMatrix();
        m_LineShader->Bind();
        m_LineShader->SetUniform("uAssembledMatrix", matrix);
        m_LineShader->SetUniform("uLineColor", LineColor);

        glDrawArrays(GL_LINES, 0, drawsize/12);
        m_LineDraws.clear();
    }}};
}
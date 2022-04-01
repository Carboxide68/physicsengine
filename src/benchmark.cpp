#include <iostream>
#include <vector>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include "shader.h"
#include "buffer.h"
#include "camera.h"

const float PI = 3.1415926535;

GLFWwindow *window;
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

Ref<Buffer> buffer;
Ref<Buffer> energy_buffer;
Ref<Buffer> sphere_buffer;

size_t nodes_size;
size_t connections_size;

size_t mesh_size = 0;

struct Node {

    glm::vec4 pos;
    glm::vec4 velocity;
    float mass;

    uint32_t locked = false;
    uint32_t padding[2] = {0, 0};

};

struct Connection {

    uint first;
    uint second;
    float neutral_length;

};

struct Accumulation {

    glm::vec4 velocity;
    glm::vec4 force;

};

int init_graphics_env();
void draw();

void buildPendulum(std::vector<Connection>& connections, std::vector<Node>& nodes);
void buildSheet(std::vector<Connection>& connections, std::vector<Node>& nodes);
void buildCube(std::vector<Connection>& connections, std::vector<Node>& nodes);

void benchmarkPendulum();
void benchmarkSheet();
void benchmarkCube();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);

void glfw_error_callback(int error, const char* description) {
    printf("Glfw Error %d: %s\n", error, description);
}

void onglerror(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userparam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH) 
        printf("OpenGL Error! Severity: High | Description: %s\n", message);
}

int main(int argc, char *argv[]) {

    auto err = init_graphics_env();
    if (err) return 0;
    glClearColor(1, 1, 1, 1);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw();

        ImGui::Begin("Settings");
        
        ImGui::InputInt("Iterations", &ITERATIONS);
        {
        int nc = node_count;
        if (ImGui::InputInt("Node count", &nc)) {
            node_count = nc;
        }
        }
        ImGui::InputFloat("Time Step", &TS);
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
        ImGui::Checkbox("Just Load", &just_load);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        FrameMark;
        TracyGpuCollect;
        glfwSwapBuffers(window);

    }

}

Camera camera = Camera();
glm::vec3 movement = glm::vec3(0);
void draw() {

    static bool initialized = false;
    static uint VAO;
    static uint mesh_VAO;
    static Ref<Shader> c_shader;
    static Ref<Shader> p_shader;
    static const glm::mat4 model_matrix(0.01);
    if (buffer_state == -1) return;
    
    if (!initialized) {
        camera.updatePerspectiveMatrix(130, 0.1f, 100.f);
        camera.setPos({-1, -1, -1});
        camera.lookAt({0, 0, 0});
        glGenVertexArrays(1, &VAO);

        glGenVertexArrays(1, &mesh_VAO);
        glBindVertexArray(mesh_VAO);
        sphere_buffer->bind(GL_ARRAY_BUFFER);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
                sizeof(float) * 6, 0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 
                sizeof(float) * 6, (void*)(sizeof(float) * 3));
        Buffer::unbind(0);
        glBindVertexArray(0);

        c_shader = Shader::Create("src/connectionShader.os");
        p_shader = Shader::Create("src/meshShader.os");

        initialized = true;
    }

    camera.move(movement * camera.getLookingMatrix());

    glBindVertexArray(mesh_VAO);
    p_shader->Bind();
    p_shader->SetUniform("u_model_matrix", model_matrix);
    p_shader->SetUniform("u_assembled_matrix", camera.getPerspectiveMatrix() * camera.getViewMatrix());
    p_shader->SetUniform("u_player_pos", camera.getPos());
    glDrawArraysInstanced(GL_TRIANGLES, 0, mesh_size/6, nodes_size/sizeof(Node));

    glBindVertexArray(VAO);
    c_shader->Bind();
    c_shader->SetUniform("u_assembled_matrix", camera.getPerspectiveMatrix() * camera.getViewMatrix());
    glDrawArrays(GL_LINES, 0, connections_size/(sizeof(Connection)) * 2);
}

#include "gltf_impl.inl"
int init_graphics_env() {

     /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); 

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return -1;
    }

    glDebugMessageCallback(onglerror, NULL);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    int max_compute[3];
    int max_compute_count[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_compute[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_compute[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &max_compute[2]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,0, &max_compute_count[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,1, &max_compute_count[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,2, &max_compute_count[2]);
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &invocations);
    printf("Compute Work Group Size: %d, %d, %d\n", max_compute[0], max_compute[1], max_compute[2]);
    printf("Compute Work Group Count: %d, %d, %d\n", max_compute_count[0], max_compute_count[1], max_compute_count[2]);
    printf("Max Work Group Invocations: %d\n", invocations);

    //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    TracyGpuContext(window);

    tinygltf_impl::TinyGLTF loader;
    tinygltf_impl::Model model;

    tinygltf_impl::load_model(loader, &model, "src/sphere.gltf");

    tinygltf_impl::Mesh mesh = model.meshes[0];
    std::vector<float> data = tinygltf_impl::MeshToFloats(model, mesh);
    sphere_buffer = Buffer::Create(data.size() * sizeof(float), GL_STATIC_DRAW);
    sphere_buffer->subData(data.data(), 0, data.size() * sizeof(float));
    mesh_size = data.size();
    return 0;

}

size_t align(size_t alignment, size_t to_be_aligned) {

    return to_be_aligned + alignment - (to_be_aligned % alignment);

}

void benchmarkPendulum() {
    printf("_______________________________________\nTest Pendulum:\n");

    std::vector<Connection> connections;
    std::vector<Node> nodes;
    {
    ZoneScopedN("Build");
    buildPendulum(connections, nodes);
    
    nodes[0].locked = true;
    }
    printf("Node Count: %lu\nConnection Count: %lu\n", nodes.size(), connections.size());

    buffer_state = 1;

    nodes_size = nodes.size() * sizeof(Node);
    connections_size = connections.size() * sizeof(Connection);
    const size_t connections_offset = align(128, nodes_size);

    const size_t n2_offset = align(128, connections_offset + connections_size);
    const size_t acc_size = nodes.size() * sizeof(Accumulation);
    const size_t acc_offset = align(128, n2_offset + nodes_size);

    const size_t forces_size = nodes.size() * sizeof(glm::vec4);
    const size_t forces_offset = align(128, acc_offset + acc_size);

    const size_t buffer_size = forces_offset + forces_size;

    buffer = Buffer::Create(buffer_size, GL_STATIC_DRAW);
    buffer->subData(nodes.data(), 0, nodes_size);
    buffer->subData(connections.data(), connections_offset, connections_size);

    //Nodes
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffer->getHandle(),
            0, nodes_size);
    //Connections
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, buffer->getHandle(), 
            connections_offset, connections_size);
    //BaseNodes
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 4, buffer->getHandle(),
            n2_offset, nodes_size);
    //Combined
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 5, buffer->getHandle(),
            acc_offset, acc_size);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 6, buffer->getHandle(),
            forces_offset, forces_size);

    energy_buffer = Buffer::Create(sizeof(float) * 3, GL_DYNAMIC_READ);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, energy_buffer->getHandle(), 0, sizeof(float) * 3);

    if (just_load) return;
    Ref<Shader> energy = Shader::Create("src/energy_shader.os");
    energy->Bind();
    glm::vec3 g = glm::vec3(0);
    if (use_gravity) {
        g = glm::vec3(0, -9.8, 0);
    }
    energy->SetUniform("gravity", g);
    energy->SetUniform("K", K);
    glDispatchCompute(1, 1, 1);
    float energies_before[3];
    energy_buffer->getContents(energies_before, 0, sizeof(float) * 3);

    Ref<Shader> compute = Shader::Create("src/physics_second.os");
    compute->Bind();
    compute->SetUniform("drag", drag);
    compute->SetUniform("gravity", g);
    compute->SetUniform("K", K);
    compute->SetUniform("TS", TS);

    {
    TracyGpuZone("Pendulum");
    ZoneScopedN("pendulum_cpu");
    
    for (int i = 0; i < ITERATIONS; i++) {
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    {
    ZoneScopedN("glFinish");
    glFinish();
    }
    TracyGpuCollect;
    }
    buffer->getContents(nodes.data(), 0, nodes_size);
    for (auto& node : nodes) {
        
        if (
            std::isnan(  node.mass       ) ||
            std::isnan(  node.pos.x      ) ||
            std::isnan(  node.pos.y      ) ||
            std::isnan(  node.pos.z      ) ||
            std::isnan(  node.pos.w      ) ||
            std::isnan(  node.velocity.x ) ||
            std::isnan(  node.velocity.y ) ||
            std::isnan(  node.velocity.z ) ||
            std::isnan(  node.velocity.w )
            ) {
        
                printf("Test \"Pendulum\" was invalid! Value found to be NaN!\n");
        }
    }

    float energies_after[3];
    energy->Bind();
    energy->SetUniform("gravity", g);
    energy->SetUniform("K", K);
    glDispatchCompute(1, 1, 1);
    energy_buffer->getContents(energies_after, 0, sizeof(float) * 3);

    float energies_diff[4];
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


void benchmarkSheet() {

    printf("_______________________________________\nTest Sheet:\n");
    
    std::vector<Connection> connections;
    std::vector<Node> nodes;
    {
    ZoneScopedN("Build");
    buildSheet(connections, nodes);
    
    const uint node_count_1D = sqrt(node_count);
    nodes[0].locked = true;
    nodes[node_count_1D-1].locked = true;
    nodes[(node_count_1D - 1) * node_count_1D].locked = true;
    nodes[node_count_1D * node_count_1D - 1].locked = true;
    }
    printf("Node Count: %lu\nConnection Count: %lu\n", nodes.size(), connections.size());

    buffer_state = 2;

    nodes_size = nodes.size() * sizeof(Node);

    connections_size = connections.size() * sizeof(Connection);
    const size_t connections_offset = align(128, nodes_size);

    const size_t n2_offset = align(128, connections_offset + connections_size);

    const size_t acc_size = nodes.size() * sizeof(Accumulation);
    const size_t acc_offset = align(128, n2_offset + nodes_size);

    const size_t forces_size = nodes.size() * sizeof(glm::vec4);
    const size_t forces_offset = align(128, acc_offset + acc_size);

    const size_t buffer_size = forces_offset + forces_size;

    buffer = Buffer::Create(buffer_size, GL_STATIC_DRAW);
    buffer->subData(nodes.data(), 0, nodes_size);
    buffer->subData(connections.data(), connections_offset, connections_size);

    //Nodes
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffer->getHandle(),
            0, nodes_size);
    //Connections
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, buffer->getHandle(), 
            connections_offset, connections_size);
    //BaseNodes
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 4, buffer->getHandle(),
            n2_offset, nodes_size);
    //Combined
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 5, buffer->getHandle(),
            acc_offset, acc_size);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 6, buffer->getHandle(),
            forces_offset, forces_size);

    energy_buffer = Buffer::Create(sizeof(float) * 3, GL_DYNAMIC_READ);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, energy_buffer->getHandle(), 0, sizeof(float) * 3);

    if (just_load) return;

    glm::vec3 g = glm::vec3(0);
    if (use_gravity) {
        g = glm::vec3(0, -9.8, 0);
    }
    Ref<Shader> energy = Shader::Create("src/energy_shader.os");
    energy->Bind();
    energy->SetUniform("gravity", g);
    energy->SetUniform("K", K);
    glDispatchCompute(1, 1, 1);
    float energies_before[3];
    energy_buffer->getContents(energies_before, 0, sizeof(float) * 3);

    Ref<Shader> compute = Shader::Create("src/physics_second.os");
    compute->Bind();
    compute->SetUniform("drag", drag);
    compute->SetUniform("gravity", g);
    compute->SetUniform("K", K);
    compute->SetUniform("TS", TS);

    {
    TracyGpuZone("Sheet");
    ZoneScopedN("sheet_cpu");
    
    for (int i = 0; i < ITERATIONS; i++) {
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    {
    ZoneScopedN("glFinish");
    glFinish();
    }
    TracyGpuCollect;
    }

    buffer->getContents(nodes.data(), 0, nodes_size);
    for (auto& node : nodes) {
        
        if (
            std::isnan(  node.mass       ) ||
            std::isnan(  node.pos.x      ) ||
            std::isnan(  node.pos.y      ) ||
            std::isnan(  node.pos.z      ) ||
            std::isnan(  node.pos.w      ) ||
            std::isnan(  node.velocity.x ) ||
            std::isnan(  node.velocity.y ) ||
            std::isnan(  node.velocity.z ) ||
            std::isnan(  node.velocity.w )
            ) {
        
                printf("Test \"Sheet\" was invalid! Value found to be NaN!\n");
        }
    }

    float energies_after[3];
    energy->Bind();
    energy->SetUniform("gravity", g);
    energy->SetUniform("K", K);
    glDispatchCompute(1, 1, 1);
    energy_buffer->getContents(energies_after, 0, sizeof(float) * 3);

    float energies_diff[4];
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

void benchmarkCube() {
    printf("_______________________________________\nTest Cube:\n");
    
    std::vector<Connection> connections;
    std::vector<Node> nodes;
    {
    ZoneScopedN("Build");
    buildCube(connections, nodes);
    
    const size_t node_count_1D = glm::pow((double)node_count, 1.0/3.0);
    for (size_t x = 0; x < node_count_1D; x++) {
        for (size_t z = 0; z < node_count_1D; z++) {
            const size_t index = (x) * node_count_1D * node_count_1D +\
                                 (0) * node_count_1D + z;
            nodes[index].locked = 1;
        }
    }
    }
    printf("Node Count: %lu\nConnection Count: %lu\n", nodes.size(), connections.size());

    buffer_state = 3;

    nodes_size = nodes.size() * sizeof(Node);
    connections_size = connections.size() * sizeof(Connection);
    const size_t connections_offset = align(128, nodes_size);

    const size_t n2_offset = align(128, connections_offset + connections_size);
    const size_t acc_size = nodes.size() * sizeof(Accumulation);
    const size_t acc_offset = align(128, n2_offset + nodes_size);

    const size_t forces_size = nodes.size() * sizeof(glm::vec4);
    const size_t forces_offset = align(128, acc_offset + acc_size);

    const size_t buffer_size = forces_offset + forces_size;

    buffer = Buffer::Create(buffer_size, GL_STATIC_DRAW);
    buffer->subData(nodes.data(), 0, nodes_size);
    buffer->subData(connections.data(), connections_offset, connections_size);

    //Nodes
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffer->getHandle(),
            0, nodes_size);
    //Connections
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, buffer->getHandle(), 
            connections_offset, connections_size);
    //BaseNodes
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 4, buffer->getHandle(),
            n2_offset, nodes_size);
    //Combined
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 5, buffer->getHandle(),
            acc_offset, acc_size);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 6, buffer->getHandle(),
            forces_offset, forces_size);

    energy_buffer = Buffer::Create(sizeof(float) * 3, GL_DYNAMIC_READ);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, energy_buffer->getHandle(), 0, sizeof(float) * 3);

    if (just_load) return;
    Ref<Shader> energy = Shader::Create("src/energy_shader.os");
    energy->Bind();
    energy->SetUniform("gravity", glm::vec3(0, -9.8, 0));
    energy->SetUniform("K", K);
    glDispatchCompute(1, 1, 1);
    float energies_before[3];
    energy_buffer->getContents(energies_before, 0, sizeof(float) * 3);

    Ref<Shader> compute = Shader::Create("src/physics_second.os");
    compute->Bind();
    compute->SetUniform("drag", drag);
    compute->SetUniform("gravity", glm::vec3(0, -9.8, 0));
    compute->SetUniform("K", K);
    compute->SetUniform("TS", TS);

    {
    TracyGpuZone("Cube");
    ZoneScopedN("cube_cpu");
    
    for (int i = 0; i < ITERATIONS; i++) {
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    {
    ZoneScopedN("glFinish");
    glFinish();
    }
    TracyGpuCollect;
    }

    buffer->getContents(nodes.data(), 0, nodes_size);
    for (auto& node : nodes) {
        
        if (
            std::isnan(  node.mass       ) ||
            std::isnan(  node.pos.x      ) ||
            std::isnan(  node.pos.y      ) ||
            std::isnan(  node.pos.z      ) ||
            std::isnan(  node.pos.w      ) ||
            std::isnan(  node.velocity.x ) ||
            std::isnan(  node.velocity.y ) ||
            std::isnan(  node.velocity.z ) ||
            std::isnan(  node.velocity.w )
            ) {
      
                printf("Test \"Cube\" was invalid! Value found to be NaN!\n");

        }
    }

    float energies_after[3];
    energy->Bind();
    energy->SetUniform("gravity", glm::vec3(0, -9.8, 0));
    energy->SetUniform("K", K);
    glDispatchCompute(1, 1, 1);
    energy_buffer->getContents(energies_after, 0, sizeof(float) * 3);

    float energies_diff[4];
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

void buildPendulum(std::vector<Connection>& connections, std::vector<Node>& nodes) {

    ZoneScopedS(5);
    static const float angle = PI/4.0;
    nodes.clear();
    nodes.reserve(node_count);
    const float fraction = size/(float)node_count;
    const float mass_fraction = mass/(float)node_count;
    const glm::vec4 d = glm::vec4(glm::cos(angle), -glm::sin(angle), 0, 0);
    const glm::vec4 direction = d * fraction;
    for (size_t i = 0; i < node_count; i++) {
        Node n;
        n.pos = direction * float(i);
        n.velocity = glm::vec4(0);
        n.mass = mass_fraction;
        n.locked = false;
        nodes.push_back(n);
    }
    connections.clear();
    connections.reserve(node_count - 1);
    for (size_t i = 0; i < node_count - 1; i++) {
        const Node& node1 = nodes[i+0];
        const Node& node2 = nodes[i+1];
        const float between = glm::distance(node1.pos, node2.pos);

        connections.emplace_back(i+0, i+1, between);
    }
}

void buildSheet(std::vector<Connection>& connections, std::vector<Node>& nodes) {

    ZoneScopedS(5);
    const size_t node_count_1D = glm::sqrt(node_count);
    const size_t nc = node_count_1D * node_count_1D;
    const float fraction = size/(float)node_count_1D;
    const float mass_fraction = mass/(float)nc;

    nodes.clear();
    nodes.reserve(nc);
    for (size_t x = 0; x < node_count_1D; x++) {
        for (size_t z = 0; z < node_count_1D; z++) {
            Node n;
            n.pos = glm::vec4(x * fraction, 0, z * fraction, 0);
            n.velocity = glm::vec4(0);
            n.mass = mass_fraction;
            n.locked = false;
            nodes.push_back(n);
        }
    }
    
    connections.clear();
    size_t counter = 0;
    for (size_t x = 1; x < node_count_1D - 1; x++) {
        for (size_t z = 1; z < node_count_1D - 1; z++) {
            std::array<uint, 4> cons;
            const size_t index = (x) * node_count_1D + (z);
            cons[0] = index + 1; //Side
            cons[1] = index + node_count_1D - 1; //Diag
            cons[2] = index + node_count_1D + 1; //Diag
            cons[3] = index + node_count_1D; //Front
            for (auto c : cons) {
                Node& node1 = nodes[index];
                Node& node2 = nodes[c];
                const float between = glm::length(node1.pos - node2.pos);

                connections.emplace_back(index, c, between);
            }
        }
        counter++;
    }
    std::vector<std::pair<uint, uint>> cons;
    //Edge cases

    for (size_t i = 0; i < node_count_1D - 1; i++) {
        size_t index = i * node_count_1D;
        cons.emplace_back(index, index+1);
        cons.emplace_back(index, index + node_count_1D + 1);
        cons.emplace_back(index, index + node_count_1D);

        index = (i+1) * node_count_1D - 1;
        cons.emplace_back(index, index + node_count_1D);
        cons.emplace_back(index, index + node_count_1D-1);
    }
    for (size_t i = 1; i < node_count_1D - 1; i++) {
        size_t index = i;
        cons.emplace_back(index, index+1);
        cons.emplace_back(index, index + node_count_1D + 1);
        cons.emplace_back(index, index + node_count_1D - 1);
        cons.emplace_back(index, index + node_count_1D);

        index = i + node_count_1D * (node_count_1D-1);
        cons.emplace_back(index, index+1);
    }

    //Edge edge cases
    cons.emplace_back(node_count_1D * (node_count_1D - 1), node_count_1D * (node_count_1D - 1) + 1);
    cons.emplace_back(node_count_1D - 1, node_count_1D * 2 - 1);
    cons.emplace_back(node_count_1D - 1, node_count_1D * 2 - 2);
    for (auto c : cons) {
        
        Node& node1 = nodes[c.first];
        Node& node2 = nodes[c.second];
        const float between = glm::length(node1.pos - node2.pos);

        connections.emplace_back(c.first, c.second, between);
    }
}

#include "cube_generation.inl"
void buildCube(std::vector<Connection>& connections, std::vector<Node>& nodes) {
    const size_t node_count_1D = glm::pow((double)node_count, 1.0/3.0);
    const float mass_fraction = mass/(float)node_count;
    GenerateNodes(nodes, connections, {node_count_1D, size, mass_fraction});
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    camera.updatePerspectiveMatrix(130.f, 0.1f, 100.0f);
}

bool mouse_focused = false;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    static bool state = true;
    const float movementspeed = 0.05;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    }
    int mod = 1;
    if (action == GLFW_PRESS) {
        mod = 1;
    }
    if (action == GLFW_RELEASE) {
        mod = 0;
    }

    switch (key) {
        case GLFW_KEY_W:
            movement.z = movementspeed * mod;
            break;
        
        case GLFW_KEY_S:
            movement.z = -movementspeed * mod;
            break;
        
        case GLFW_KEY_A:
            movement.x = movementspeed * mod;
            break;
        
        case GLFW_KEY_D:
            movement.x = -movementspeed * mod;
            break;
        
        case GLFW_KEY_Q:
            movement.y = movementspeed * mod;
            break;

        case GLFW_KEY_E:
            movement.y = -movementspeed * mod;
            break;

        case GLFW_KEY_SPACE:
            if (mod == 0) break;
            if (mouse_focused) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                mouse_focused = false;
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                mouse_focused = true;
            }
            break;

        case GLFW_KEY_LEFT_ALT:
            if (mod == 0) break;
            if (state) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                state = false;
            }
            else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
                state = true;
            }
            break;

        default:
            break;
    }

}

double lastXPos = 0;
double lastYPos = 0;
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos) {
    double xPosDiff = xPos - lastXPos;
    double yPosDiff = yPos - lastYPos;

    lastXPos = xPos;
    lastYPos = yPos;

    if (!mouse_focused) return;

    camera.rotateY(-xPosDiff/1000);
    camera.rotateX(yPosDiff/700);
}

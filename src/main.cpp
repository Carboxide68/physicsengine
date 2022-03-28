#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "common.h"
#include "buffer.h"
#include "shader.h"
#include "camera.h"

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <tiny_gltf/tiny_gltf.h>

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include <random>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);

GLFWwindow *window;
Camera camera;
double lastXPos = 0;
double lastYPos = 0;

bool mouse_focused = false;
const float FOV = 130.0;
glm::vec3 movement = glm::vec3(0);

float drag = 0.1;
glm::vec3 gravity = glm::vec3(0, -9.8, 0);
float K = 98;
float TS = 0.01;

void glfw_error_callback(int error, const char* description) {
    printf("Glfw Error %d: %s\n", error, description);
}

void onglerror(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userparam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH) 
        printf("OpenGL Error! Severity: High | Description: %s\n", message);
}

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
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    #ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    #endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    TracyGpuContext(window);

    return 0;

}

namespace tinygltf_impl {
    using namespace tinygltf;
    int load_model(TinyGLTF &loader, Model *model, std::string model_name) {
        std::string err;
        std::string warn;

        bool ret = loader.LoadASCIIFromFile(model, &err, &warn, model_name);

        if (!warn.empty()) {
          printf("Warn: %s\n", warn.c_str());
        }

        if (!err.empty()) {
          printf("Err: %s\n", err.c_str());
        }

        if (!ret) {
          printf("Failed to parse glTF\n");
          return -1;
        }
        return 0;
    }
    
    std::vector<float> MeshToFloats(Model& model, Mesh& mesh) {
        
        std::vector<float> floats;
        BufferView index_buffer = model.bufferViews[model.accessors[mesh.primitives[0].indices].bufferView];
        uint16_t *indices = (uint16_t*)((size_t)model.buffers[index_buffer.buffer].data.data() + (size_t)index_buffer.byteOffset);

        BufferView normal_buffer = model.bufferViews[model.accessors[mesh.primitives[0].attributes["NORMAL"]].bufferView];
        float *normals = (float*)((size_t)model.buffers[normal_buffer.buffer].data.data() + (size_t)normal_buffer.byteOffset);

        BufferView position_buffer = model.bufferViews[model.accessors[mesh.primitives[0].attributes["POSITION"]].bufferView];
        float *positions = (float*)((size_t)model.buffers[position_buffer.buffer].data.data() + (size_t)position_buffer.byteOffset);

        for (uint16_t *i = indices; (size_t)i < (size_t)indices + (size_t)index_buffer.byteLength; i++) {
            floats.push_back(positions[(*i)*3]);
            floats.push_back(positions[(*i)*3+1]);
            floats.push_back(positions[(*i)*3+2]);

            floats.push_back(normals[(*i)*3]);
            floats.push_back(normals[(*i)*3+1]);
            floats.push_back(normals[(*i)*3+2]);
        }
        return floats;
    }
}

class VoidData {
public:

    VoidData(size_t size) {
        start = malloc(size);
        head = start;
        end = start + size;
    }    

    ~VoidData() {
        free(start);
    }

    template <typename T>
    void addData(T data) {    
        if (head + sizeof(T) > end) {printf("Reached maximum capacity!\n");return;}
        *((T*)head) = data;
        head += sizeof(T);
    }

    void begin(const std::string& label) {
        labels[label] = (size_t)head - (size_t)start;     
    }

    std::unordered_map<std::string, size_t> labels;

    void *head;
    void *start;
    void *end;

};

struct Node {

    glm::vec4 pos;
    glm::vec4 velocity;
    float mass;

    int32_t connections[30];
    uint32_t locked = false;

};

struct Connection {

    uint first;
    uint second;
    float normal_length;

};



struct NodeConfig {

    uint box_size;
    float box_extent;
    float mass;

};

#include "cube_generation.inl"

int main(int argc, char *argv[]) {
    
    glfwSetErrorCallback(glfw_error_callback);
    if (init_graphics_env() == -1) {
        printf("Couldn't properly create graphics a environment!\n");
        return -1;
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    camera = Camera();
    camera.updatePerspectiveMatrix(FOV, 0.1f, 100.0f);
    
    uint VAO;
    uint connection_VAO;
    glGenVertexArrays(1, &VAO);
    glGenVertexArrays(1, &connection_VAO);
    glBindVertexArray(VAO);

    tinygltf_impl::TinyGLTF loader;
    tinygltf_impl::Model model;

    tinygltf_impl::load_model(loader, &model, "src/sphere.gltf");
    tinygltf_impl::Mesh mesh = model.meshes[0];
    std::vector<float> data = tinygltf_impl::MeshToFloats(model, mesh);
    Ref<Buffer> buffer = Buffer::Create(data.size() * sizeof(float), GL_STATIC_DRAW);
    buffer->subData(data.data(), 0, data.size() * sizeof(float));

    buffer->bind(GL_ARRAY_BUFFER);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));
    Buffer::unbind(GL_ARRAY_BUFFER);
    glBindVertexArray(0);

    Ref<Shader> shader = Shader::Create("src/meshShader.os");
    Ref<Shader> c_shader = Shader::Create("src/connectionShader.os");
    Ref<Shader> point_shader = Shader::Create("src/point_shader.os");
    Ref<Shader> physics_compute = Shader::Create("src/physics.os");
    Ref<Shader> energy_compute = Shader::Create("src/energy_shader.os");
    glm::mat4 model_matrix = glm::mat4(1);

    float scale = 0.5;

    NodeConfig config = {25, 5.0f, 1.f/25.f};
    std::vector<Node> nodes;
    std::vector<Connection> connections;

    GenerateNodes(nodes, connections, config);
    for (uint x = 0; x < config.box_size; x++) {
        for (uint z = 0; z < config.box_size; z++) {
            const size_t node_count = config.box_size;
            //const uint y = node_count - 1;
            const uint y = 0;
            const size_t index = x * node_count * node_count + y * node_count + z;
            nodes[index].locked = 1;
        }
    }

    VoidData node_buffer = VoidData(sizeof(Node) * nodes.size());
    for (auto& node : nodes) {
        node_buffer.addData(node);
    }

    Ref<Buffer> nb = Buffer::Create(sizeof(Node) * nodes.size(), GL_STATIC_DRAW);
    printf("Allocated %luB data in GPU memory!\n", sizeof(Node) * nodes.size());
    nb->subData(node_buffer.start, 0, sizeof(Node) * nodes.size());
    glBindBufferRange( GL_SHADER_STORAGE_BUFFER, 0, nb->getHandle(), 0, sizeof(Node) * nodes.size());


    Ref<Buffer> cb = Buffer::Create(sizeof(Connection) * connections.size(), GL_STATIC_DRAW);
    printf("Allocated %luB data in GPU memory!\n", sizeof(Connection) * connections.size());
    cb->subData(&connections[0].first, 0, sizeof(Connection) * connections.size());
    glBindBufferRange( GL_SHADER_STORAGE_BUFFER, 1, cb->getHandle(), 0, sizeof(Connection) * connections.size());

    Ref<Buffer> eb = Buffer::Create(sizeof(float) * 3, GL_STREAM_READ);
    glBindBufferRange( GL_SHADER_STORAGE_BUFFER, 2, eb->getHandle(), 0, sizeof(float) * 3);

    camera.setPos({5, 5, 5});
    
    glClearColor(1, 1, 1, 1);
    if (argc > 1) {
        printf("argv[1]: %s\n", argv[1]);
        if (!std::strcmp(argv[1], "stop")) return 0;
    }

    camera.lookAt({0, 0, 0});

    bool draw_connections = false;
    bool draw_points = false;

    bool calc_energy = false;

    int doTick = 0;
    float standard_height_energy = -100001;

    struct Statistics {
        
        uint64_t tick_count;
        float sim_time;

    };        
    Statistics stats = {0, 0.0f};
    int ticks_per_frame = 1;
    float energies[3] = {0.f, 0.f, 0.f};

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        camera.move(movement * camera.getLookingMatrix());

        //Computing
        
        if (doTick) {
        
            doTick--;
            physics_compute->Bind();
            physics_compute->SetUniform("drag", drag);
            physics_compute->SetUniform("gravity", gravity);
            physics_compute->SetUniform("K", K);
            physics_compute->SetUniform("TS", TS);
            const auto node_count = config.box_size;
            for (uint i = 0; i < ticks_per_frame; i++) {
                glDispatchCompute(node_count, node_count, node_count);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                stats.tick_count++;
                stats.sim_time += TS;
            }
        }
        if (calc_energy) {
            energy_compute->Bind();
            energy_compute->SetUniform("K", K);
            energy_compute->SetUniform("gravity", gravity);
            glDispatchCompute(2, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            eb->bind(GL_SHADER_STORAGE_BUFFER);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * 3, &energies);
            if (standard_height_energy == -100001) standard_height_energy = energies[2];
            energies[2] -= standard_height_energy;
        }


        //Drawing

        if (draw_points) {
            glBindVertexArray(connection_VAO);
            point_shader->Bind();
            point_shader->SetUniform("u_assembled_matrix", camera.getPerspectiveMatrix() * camera.getViewMatrix());
            point_shader->SetUniform("u_node_size", model_matrix[0][0]);
            glDrawArrays(GL_POINTS, 0, nodes.size());
        } else {
            glBindVertexArray(VAO);
            shader->Bind();
            shader->SetUniform("u_model_matrix", model_matrix);
            shader->SetUniform("u_assembled_matrix", camera.getPerspectiveMatrix() * camera.getViewMatrix());
            shader->SetUniform("u_player_pos", camera.getPos());
            glDrawArraysInstanced(GL_TRIANGLES, 0, data.size()/6, nodes.size());
        }

        if (draw_connections) {
            glBindVertexArray(connection_VAO);
            c_shader->Bind();
            c_shader->SetUniform("u_assembled_matrix", camera.getPerspectiveMatrix() * camera.getViewMatrix());
            glDrawArrays(GL_LINES, 0, connections.size() * 2);
            glBindVertexArray(0);
        }

        ImGui::Begin("Window");
        if(ImGui::Button("Run")) doTick = -1; ImGui::SameLine();
        if(ImGui::Button("Stop")) doTick = 0; ImGui::SameLine();
        if(ImGui::Button("Single Tick")) doTick = 1;

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Tick count: %lu\nSimulated time: %fs\nTPS: %f", stats.tick_count, stats.sim_time, ticks_per_frame * ImGui::GetIO().Framerate);
        ImGui::Text("XPos: %f, YPos: %f", lastXPos, lastYPos);

        ImGui::Text("Kinetic Energy: %f\nSpring Energy: %f\nGravitational Energy: %f\nTotal Energy: %f",
                energies[0], 
                energies[1],
                energies[2], 
                energies[0] + energies[1] + energies[2]);

        if (ImGui::SliderFloat("Scale", &scale, 0.01, 1, "%.3f")) {
            model_matrix[0][0] = scale;
            model_matrix[1][1] = scale;
            model_matrix[2][2] = scale;
        }
        ImGui::Checkbox("Draw Connections", &draw_connections);
        ImGui::Checkbox("Draw Points", &draw_points);
        ImGui::Checkbox("Calculate Energy", &calc_energy);

        ImGui::InputFloat3("Gravity", glm::value_ptr(gravity));
        ImGui::SliderFloat("Drag", &drag, 0.0f, 1.0f, "%.6f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SliderFloat("K", &K, 1.0f, 100000.0f, "%.6f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SliderFloat("Tick Speed", &TS, 0.0001f, 0.1f, "%.5f");
        ImGui::InputInt("Ticks Per Frame", &ticks_per_frame);

        ImGui::End();
        

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        FrameMark;
        TracyGpuCollect;
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &VAO);

    glfwDestroyWindow(window);
    glfwTerminate();
   
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    camera.updatePerspectiveMatrix(FOV, 0.1, 100.0);
}

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

void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos) {
    double xPosDiff = xPos - lastXPos;
    double yPosDiff = yPos - lastYPos;

    lastXPos = xPos;
    lastYPos = yPos;

    if (!mouse_focused) return;

    camera.rotateY(-xPosDiff/1000);
    camera.rotateX(yPosDiff/700);
}

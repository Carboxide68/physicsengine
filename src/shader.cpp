#include "shader.h"

#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

Ref<Shader> Shader::Create(const std::string& name, const std::string& filePath) {
    return Ref<Shader>(new Shader(name, filePath));
}

Ref<Shader> Shader::Create(const std::string& filePath) {
    return Ref<Shader>(new Shader(filePath));
}

Shader::Shader(const std::string& name, const std::string& filePath) {

    m_Name = name;
    printf("Reading shader file: %s\n", filePath.c_str());
    ReadShaders(filePath);

}

Shader::Shader(const std::string& filePath) {

    printf("Reading shader file: %s\n", filePath.c_str());
    ReadShaders(filePath);

}

Shader::~Shader() {

    glDeleteProgram(m_ProgramID);

}

const std::string& Shader::GetName() const {return m_Name;}

void Shader::Bind() {
    glUseProgram(m_ProgramID);
}

void Shader::Unbind() {
    glUseProgram(0);
}

void Shader::SetUniform(const std::string& name, const bool& value) {
    glUniform1i(GetUniformLocation(name), value);
}
void Shader::SetUniform(const std::string& name, const int& value) {
    glUniform1i(GetUniformLocation(name), value);
}
void Shader::SetUniform(const std::string& name, const uint& value) {
    glUniform1ui(GetUniformLocation(name), value);
}
void Shader::SetUniform(const std::string& name, const float& value) {
    glUniform1f(GetUniformLocation(name), value);
}
void Shader::SetUniform(const std::string& name, const glm::vec2& value) {
    glUniform2f(GetUniformLocation(name), value.x, value.y);
}
void Shader::SetUniform(const std::string& name, const glm::vec3& value) {
    glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}
void Shader::SetUniform(const std::string& name, const glm::vec4& value) {
    glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}
void Shader::SetUniform(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

int32_t Shader::GetUniformLocation(const std::string& name) {
    return glGetUniformLocation(m_ProgramID, name.c_str());
}

void Shader::ReadShaders(const std::string& filePath) {

    //ZoneScopedN("Read Shader");
    std::ifstream file(filePath);

    std::string fileContents;

    if (file) {
        std::stringstream ss;
        ss << file.rdbuf();
        file.close();
        fileContents = ss.str();
    } else {
        printf("Couldn't read file %s!\n", filePath.c_str()); 
        return;
    }

    std::unordered_map<GLenum, std::string> splitFile;
    PreProcess(fileContents, splitFile);
    Compile(splitFile);

}

std::unordered_map<GLenum, std::string> Shader::PreProcess(const std::string& fileContent, std::unordered_map<GLenum, std::string>& splitFile) {
    //ZoneScopedN("Preprocess shader")
    uint pos = 0;
    GLenum curShaderType = GL_FALSE;
    while (pos < fileContent.size()) {
        if (fileContent[pos] == '@') {
            std::string tempShaderType;
            pos++;
            for (; pos < fileContent.size() && fileContent[pos] != '\n' && fileContent[pos] != ' '; pos++) {
                tempShaderType.push_back(fileContent[pos]);
            }
            pos++;
            if (tempShaderType == "vertex") {
                curShaderType = GL_VERTEX_SHADER;
            } else if (tempShaderType == "compute") {
                curShaderType = GL_COMPUTE_SHADER;
            } else if (tempShaderType == "geometry") {
                curShaderType = GL_GEOMETRY_SHADER;
            } else if (tempShaderType == "fragment") {
                curShaderType = GL_FRAGMENT_SHADER;
            } else if (tempShaderType == "end") {
                break;
            } else {
                printf("This shadertype is not supported!\n");
            }
            continue;
        }
        if (fileContent[pos] == '/' && fileContent[pos+1] == '/') continue;
        for (;pos < fileContent.size() && fileContent[pos] != '\n'; pos++) {
            splitFile[curShaderType].push_back(fileContent[pos]);
        }
        splitFile[curShaderType].push_back('\n');
        pos++;
    }
    return splitFile;
}

void Shader::Compile(const std::unordered_map<GLenum, std::string>& splitFile) { //TODO: Fix more shader types
    //ZoneScopedN("Compile shader")
    //TracyGpuZone("Compile shader")
    
    if (splitFile.find(GL_COMPUTE_SHADER) != splitFile.end()) {
        GLuint cShader = glCreateShader(GL_COMPUTE_SHADER);
        const char *cShaderSourceStr = splitFile.at(GL_COMPUTE_SHADER).c_str();
        glShaderSource(cShader, 1, &cShaderSourceStr, NULL);
        glCompileShader(cShader);
        GLint computeCompiled;
        glGetShaderiv(cShader, GL_COMPILE_STATUS, &computeCompiled);
        if (computeCompiled != GL_TRUE) {
            GLsizei log_length = 0;
            GLchar message[1024];
            glGetShaderInfoLog(cShader, 1024, &log_length, message);
            printf("Compute error: %s\n", message);
        }
        m_ProgramID = glCreateProgram();
        glAttachShader(m_ProgramID, cShader);

        glLinkProgram(m_ProgramID);

        GLint programLinked;
        glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &programLinked);
        if (programLinked != GL_TRUE) {
            GLsizei log_length = 0;
            GLchar message[1024];
            glGetProgramInfoLog(m_ProgramID, 1024, &log_length, message);
            printf("Program linkingrror: %s\n", message);
        }
        glDeleteShader(cShader);
        return;
    }
    bool usingGeom = splitFile.find(GL_GEOMETRY_SHADER) != splitFile.end();

    if (splitFile.find(GL_VERTEX_SHADER) == splitFile.end()) {
        printf("You must have a vertex shader in your program!\n");
        return;
    }
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    const char *vShaderSourceStr = splitFile.at(GL_VERTEX_SHADER).c_str();
    glShaderSource(vShader, 1, &vShaderSourceStr, NULL);
    if (splitFile.find(GL_FRAGMENT_SHADER) == splitFile.end()) {
        printf("You must have a fragment shader in your program!\n");
        return;
    }
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fShaderSourceStr = splitFile.at(GL_FRAGMENT_SHADER).c_str();
    glShaderSource(fShader, 1, &fShaderSourceStr, NULL);

    GLuint gShader;
    if (usingGeom) {
        gShader = glCreateShader(GL_GEOMETRY_SHADER);
        const char *gShaderSourceStr = splitFile.at(GL_GEOMETRY_SHADER).c_str();
        glShaderSource(gShader, 1, &gShaderSourceStr, NULL);
    }

    glCompileShader(vShader);
    GLint vertexCompiled;
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &vertexCompiled);
    if (vertexCompiled != GL_TRUE) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetShaderInfoLog(vShader, 1024, &log_length, message);
        printf("Vertex error: %s\n", message);
    }

    glCompileShader(fShader);
    GLint fragmentCompiled;
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &fragmentCompiled);
    if (fragmentCompiled != GL_TRUE) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetShaderInfoLog(fShader, 1024, &log_length, message);
        printf("Fragment error: %s\n", message);
    }

    m_ProgramID = glCreateProgram();
    glAttachShader(m_ProgramID, vShader);
    glAttachShader(m_ProgramID, fShader);

    if (usingGeom) {
        glCompileShader(gShader);
        GLint geometryCompiled;
        glGetShaderiv(gShader, GL_COMPILE_STATUS, &geometryCompiled);
        if (geometryCompiled != GL_TRUE) {
            GLsizei log_length = 0;
            GLchar message[1024];
            glGetShaderInfoLog(gShader, 1024, &log_length, message);
            printf("Geometry error: %s\n", message);
        }
        glAttachShader(m_ProgramID, gShader);
    }

    glLinkProgram(m_ProgramID);

    GLint programLinked;
    glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &programLinked);
    if (programLinked != GL_TRUE) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetProgramInfoLog(m_ProgramID, 1024, &log_length, message);
        printf("Program linkingrror: %s\n", message);
    }
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    if (usingGeom)
        glDeleteShader(gShader);

    return;
}

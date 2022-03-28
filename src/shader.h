#pragma once

#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <GL/glew.h>
#include "common.h"


class Shader {

public:

	~Shader();

	const std::string& GetName() const;

	void Bind();
	void Unbind();

	void SetUniform(const std::string& name, const bool& value);
	void SetUniform(const std::string& name, const int& value);
	void SetUniform(const std::string& name, const uint& value);
	void SetUniform(const std::string& name, const float& value);
	void SetUniform(const std::string& name, const glm::vec2& value);
	void SetUniform(const std::string& name, const glm::vec3& value);
	void SetUniform(const std::string& name, const glm::vec4& value);
	void SetUniform(const std::string& name, const glm::mat4& value);

	inline uint32_t GetHandle() {return m_ProgramID;}

	static Ref<Shader> Create(const std::string& name, const std::string& filePath);
	static Ref<Shader> Create(const std::string& filePath);

private:

	Shader(const std::string& name, const std::string& filePath);

	Shader(const std::string& filePath);

	void ReadShaders(const std::string& filePath);
	std::unordered_map<GLenum, std::string> PreProcess(const std::string& fileContent, std::unordered_map<GLenum, std::string>& splitFile);
	void Compile(const std::unordered_map<GLenum, std::string>& sources);

	int32_t GetUniformLocation(const std::string& name);

	std::string m_Name;
	std::unordered_map<std::string, int32_t> m_UniformLocations;
	uint32_t m_ProgramID;

};

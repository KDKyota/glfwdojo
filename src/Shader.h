#pragma once

# define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <string>

namespace gl{

class Shader
{
public:
	unsigned int ID;
	~Shader();

	// ジオメトリシェーダーあり版コンストラクタ
	Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath);

	// constructor generates the shader on the fly
	Shader(const char* vertexPath, const char* fragmentPath);

	// activate the shader
	// ------------------------------------------------------------------------
	void use() const;
	// utility uniform functions
	// ------------------------------------------------------------------------
	void setBool(const std::string& name, bool value) const;
	// ------------------------------------------------------------------------
	void setInt(const std::string& name, int value) const;
	// ------------------------------------------------------------------------
	void setFloat(const std::string& name, float value) const;
	// ------------------------------------------------------------------------
	void setVec2(const std::string& name, const glm::vec2& value) const;
	void setVec2(const std::string& name, float x, float y) const;
	// ------------------------------------------------------------------------
	void setVec3(const std::string& name, const glm::vec3& value) const;
	void setVec3(const std::string& name, float x, float y, float z) const;
	// ------------------------------------------------------------------------
	void setVec4(const std::string& name, const glm::vec4& value) const;
	void setVec4(const std::string& name, float x, float y, float z, float w) const;
	// ------------------------------------------------------------------------
	void setMat2(const std::string& name, const glm::mat2& mat) const;
	// ------------------------------------------------------------------------
	void setMat3(const std::string& name, const glm::mat3& mat) const;
	// ------------------------------------------------------------------------
	void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
	// utility functions for checking shader compilation/linking errors.
	void checkCompileErrors(unsigned int shader, std::string type);
};
}

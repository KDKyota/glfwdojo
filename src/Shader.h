#pragma once

#define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <string>

namespace gl {

/**
 * @brief シェーダーのコンパイル・リンクと uniform 設定を行うクラス。
 */
class Shader {
  public:
    unsigned int ID;
    ~Shader();

    /**
     * @brief Vertex/Geometry/Fragment シェーダーからプログラムを生成する。
     *
     * @param vertexPath Vertex Shader のパス。
     * @param geometryPath Geometry Shader のパス。
     * @param fragmentPath Fragment Shader のパス。
     */
    Shader(const char *vertexPath, const char *geometryPath, const char *fragmentPath);

    /**
     * @brief Vertex/Fragment シェーダーからプログラムを生成する。
     *
     * @param vertexPath Vertex Shader のパス。
     * @param fragmentPath Fragment Shader のパス。
     */
    Shader(const char *vertexPath, const char *fragmentPath);

    /**
     * @brief このプログラムをバインドする。
     */
    void use() const;

    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const;
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    void setVec2(const std::string &name, float x, float y) const;
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const;
    void setVec4(const std::string &name, float x, float y, float z, float w) const;
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat) const;
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat) const;
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat) const;

    void setMat4Array(const std::string &name, const glm::mat4 *mats, int count) const;

  private:
    // utility functions for checking shader compilation/linking errors.
    void checkCompileErrors(unsigned int shader, std::string type);
};
} // namespace gl

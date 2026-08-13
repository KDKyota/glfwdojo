#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

namespace gl {

namespace {

// GLSL に #include は無いので、読み込み時に自前で展開する
std::string expandIncludes(const std::string &path, int depth = 0) {
  if (depth > 8) {
    std::cout << "ERROR::SHADER::INCLUDE_TOO_DEEP: " << path << std::endl;
    return {};
  }
  std::ifstream file(path);
  if (!file) {
    std::cout << "ERROR::SHADER::FILE_NOT_FOUND: " << path << std::endl;
    return {};
  }

  // include されるファイルは include する側と同じフォルダに置く前提
  const std::size_t slash = path.find_last_of("/\\");
  const std::string dir =
      (slash == std::string::npos) ? std::string{} : path.substr(0, slash + 1);

  std::ostringstream out;
  std::string line;
  int lineNo = 0;
  while (std::getline(file, line)) {
    ++lineNo;
    const std::size_t head = line.find_first_not_of(" \t");
    if (head == std::string::npos || line.compare(head, 8, "#include") != 0) {
      out << line << '\n';
      continue;
    }

    const std::size_t open = line.find('"', head + 8);
    const std::size_t close =
        (open == std::string::npos) ? std::string::npos : line.find('"', open + 1);
    if (close == std::string::npos) {
      std::cout << "ERROR::SHADER::BAD_INCLUDE: " << path << ":" << lineNo
                << std::endl;
      continue;
    }

    out << expandIncludes(dir + line.substr(open + 1, close - open - 1),
                          depth + 1);
    // 展開後に行番号を戻さないと、以降のコンパイルエラーの行が全部ずれる
    out << "#line " << (lineNo + 1) << '\n';
  }
  return out.str();
}

} // namespace

Shader::~Shader() { glDeleteProgram(ID); }

Shader::Shader(const char *vertexPath, const char *geometryPath,
               const char *fragmentPath) {
  const std::string vertexCode = expandIncludes(vertexPath);
  const std::string geometryCode = expandIncludes(geometryPath);
  const std::string fragmentCode = expandIncludes(fragmentPath);

  const char *vCode = vertexCode.c_str();
  const char *gCode = geometryCode.c_str();
  const char *fCode = fragmentCode.c_str();

  unsigned int vertex, geometry, fragment;
  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vCode, NULL);
  glCompileShader(vertex);
  checkCompileErrors(vertex, "VERTEX");

  geometry = glCreateShader(GL_GEOMETRY_SHADER);
  glShaderSource(geometry, 1, &gCode, NULL);
  glCompileShader(geometry);
  checkCompileErrors(geometry, "GEOMETRY");

  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fCode, NULL);
  glCompileShader(fragment);
  checkCompileErrors(fragment, "FRAGMENT");

  ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, geometry);
  glAttachShader(ID, fragment);
  glLinkProgram(ID);
  checkCompileErrors(ID, "PROGRAM");
  glDeleteShader(vertex);
  glDeleteShader(geometry);
  glDeleteShader(fragment);
}

Shader::Shader(const char *vertexPath, const char *fragmentPath) {
  const std::string vertexCode = expandIncludes(vertexPath);
  const std::string fragmentCode = expandIncludes(fragmentPath);

  const char *vShaderCode = vertexCode.c_str();
  const char *fShaderCode = fragmentCode.c_str();
  // compile shaders
  unsigned int vertex, fragment;
  // vertex shader
  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vShaderCode, NULL);
  glCompileShader(vertex);
  checkCompileErrors(vertex, "VERTEX");
  // fragment shader
  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fShaderCode, NULL);
  glCompileShader(fragment);
  checkCompileErrors(fragment, "FRAGMENT");
  // shader program
  ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, fragment);
  glLinkProgram(ID);
  checkCompileErrors(ID, "PROGRAM");

  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

void Shader::use() const { glUseProgram(ID); }

void Shader::setBool(const std::string &name, bool value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const {
  glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec2(const std::string &name, const glm::vec2 &value) const {
  glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::setVec2(const std::string &name, float x, float y) const {
  glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) const {
  glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::setVec3(const std::string &name, float x, float y, float z) const {
  glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value) const {
  glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::setVec4(const std::string &name, float x, float y, float z,
                     float w) const {
  glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}

void Shader::setMat2(const std::string &name, const glm::mat2 &mat) const {
  glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                     glm::value_ptr(mat));
}

void Shader::setMat3(const std::string &name, const glm::mat3 &mat) const {
  glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                     glm::value_ptr(mat));
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const {
  glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                     glm::value_ptr(mat));
}

void Shader::checkCompileErrors(unsigned int shader, std::string type) {
  int success;
  char infoLog[512];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 512, NULL, infoLog);
      std::cout
          << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- "
          << std::endl;
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 512, NULL, infoLog);
      std::cout
          << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- "
          << std::endl;
    }
  }
}

} // namespace gl

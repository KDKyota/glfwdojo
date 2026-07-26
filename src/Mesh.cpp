#include "Mesh.h"

Mesh::Mesh(std::vector<gl::Vertex> vertices, std::vector<unsigned int> indices, Material material)
	: vertices_(std::move(vertices)), indices_(std::move(indices)), material_(std::move(material))
{
	setupMesh();
}

Mesh::~Mesh()
{
	if(VAO_ !=0) glDeleteVertexArrays(1, &VAO_);
	if(VBO_ !=0) glDeleteBuffers(1, &VBO_);
	if(EBO_ !=0) glDeleteBuffers(1, &EBO_);
}

void Mesh::setupMesh()
{
	glGenVertexArrays(1, &VAO_);
	glGenBuffers(1, &VBO_);
	glGenBuffers(1, &EBO_);

	glBindVertexArray(VAO_);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_);

	glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(gl::Vertex), vertices_.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(gl::Vertex), (void*)offsetof(gl::Vertex, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(gl::Vertex), (void*)offsetof(gl::Vertex, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(gl::Vertex), (void*)offsetof(gl::Vertex, uv));

	glBindVertexArray(0);
}

Mesh::Mesh(Mesh&& other) noexcept
      : vertices_(std::move(other.vertices_)),
        indices_(std::move(other.indices_)),
        material_(std::move(other.material_)),
        VAO_(other.VAO_), VBO_(other.VBO_), EBO_(other.EBO_)
{
      other.VAO_ = other.VBO_ = other.EBO_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
      if (this != &other) {
              if (VAO_ != 0) glDeleteVertexArrays(1, &VAO_);
              if (VBO_ != 0) glDeleteBuffers(1, &VBO_);
              if (EBO_ != 0) glDeleteBuffers(1, &EBO_);

              vertices_ = std::move(other.vertices_);
              indices_ = std::move(other.indices_);
              material_ = std::move(other.material_);
              VAO_ = other.VAO_;
              VBO_ = other.VBO_;
              EBO_ = other.EBO_;

              other.VAO_ = other.VBO_ = other.EBO_ = 0;
      }
      return *this;
}

void Mesh::Draw(gl::Shader& shader) const
{
	material_.bind();
	material_.setUniforms(shader);
	glBindVertexArray(VAO_);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

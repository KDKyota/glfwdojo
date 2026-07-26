#pragma once
#include <vector>
#include "Material.h"
#include "GeometryData.h"
#include "Texture.h"


class Mesh {
private:
	unsigned int VAO_, VBO_, EBO_;
	std::vector<gl::Vertex> vertices_;
	std::vector<unsigned int> indices_;
	Material material_;
	std::vector<Texture> textures_; // textureを持つマテリアル

	void setupMesh();
public:

	Mesh(std::vector<gl::Vertex> vertices, std::vector<unsigned int> indices, Material material);
	~Mesh();

	// コピー禁止
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	// ムーブ・右辺値代入は許可（shared_ptr内部で利用される）
	Mesh(Mesh&& other) noexcept;
	Mesh& operator=(Mesh&& other) noexcept;

	void Draw(gl::Shader& shader) const;
};
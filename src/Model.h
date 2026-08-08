#pragma once
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "TextureCache.h"
#include "Mesh.h"

class Model {
private:
	std::vector<Mesh> meshes_; // model data
	std::string directory_;
	TextureCache& cache_;

	void loadModel(const std::string& path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	Material loadMaterial(aiMaterial* mat);
	std::shared_ptr<Texture> loadMaterialTexture(aiMaterial* Mat, aiTextureType type, bool flip,
	                                             ColorSpace colorSpace);

public:
	Model(const std::string& path, TextureCache& cache);
	void Draw(gl::Shader& shader) const;

};
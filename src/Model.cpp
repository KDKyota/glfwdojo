#include "Model.h"
#include <stdexcept>

Model::Model(const std::string& path, TextureCache& cache) : cache_(cache)
{
	loadModel(path);
}

void Model::loadModel(const std::string& path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		throw std::runtime_error("ERROR::ASSIMP::" + std::string(importer.GetErrorString()));
	}
	const size_t slash = path.find_last_of("/\\");
	directory_ = (slash == std::string::npos) ? "" : path.substr(0, slash);
	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes_.push_back(processMesh(mesh, scene));			
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}  

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<gl::Vertex> vertices;
	vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
		gl::Vertex vertex;
		vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
	
		if (mesh->HasNormals())
		{
			vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		}
		// texture coordinates
		if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertex.uv = glm::vec2(0.0f, 0.0f);
		}
		vertices.push_back(vertex);
    }
    // process indices
	std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
    }

	Material material = loadMaterial(scene->mMaterials[mesh->mMaterialIndex]);
	return Mesh(std::move(vertices), std::move(indices), std::move(material));

}	
	

Material Model::loadMaterial(aiMaterial* mat)
{
	Material material;
	material.diffuse = loadMaterialTexture(mat, aiTextureType_DIFFUSE, false, ColorSpace::SRGB);
	material.specular = loadMaterialTexture(mat, aiTextureType_SPECULAR, false, ColorSpace::Linear);
	return material;
}

std::shared_ptr<Texture> Model::loadMaterialTexture(aiMaterial* mat, aiTextureType type, bool flip,
                                                    ColorSpace colorSpace)
{
	if (mat->GetTextureCount(type) == 0) {
		return nullptr;
	}

	aiString str;
	mat->GetTexture(type, 0, &str);

	const std::string path = directory_ + "/" + str.C_Str();
	return cache_.get(path, flip, colorSpace);
}

void Model::Draw(gl::Shader& shader) const {
	for (const auto& mesh : meshes_) {
		mesh.Draw(shader);
	}
}

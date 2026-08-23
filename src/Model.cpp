#include "Model.h"

#include <iostream>
#include <stdexcept>

namespace {

// aiMatrix4x4 は行優先、glm::mat4 は列優先なので転置しながら詰め替える
glm::mat4 toGlm(const aiMatrix4x4& m)
{
	return glm::mat4(m.a1, m.b1, m.c1, m.d1,
	                 m.a2, m.b2, m.c2, m.d2,
	                 m.a3, m.b3, m.c3, m.d3,
	                 m.a4, m.b4, m.c4, m.d4);
}

void addBoneInfluence(gl::Vertex& vertex, int boneIndex, float weight)
{
	if (weight <= 0.0f) {
		return;
	}
	for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
		if (vertex.m_Weights[i] == 0.0f) {
			vertex.m_BoneIDs[i] = boneIndex;
			vertex.m_Weights[i] = weight;
			return;
		}
	}
	// aiProcess_LimitBoneWeights で 4 本へ切り詰めているので、あふれてここへ来ることはない
}

} // namespace

Model::Model(const std::string& path, TextureCache& cache) : path_(path), cache_(cache)
{
	loadModel(path);
}

void Model::loadModel(const std::string& path)
{
	Assimp::Importer importer;
	// LimitBoneWeights は1頂点あたりの影響を4本へ切り詰める gl::Vertex の枠と一致させるため必須
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs |
	                                                   aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals |
	                                                   aiProcess_LimitBoneWeights);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		throw std::runtime_error("ERROR::ASSIMP::" + std::string(importer.GetErrorString()));
	}
	const size_t slash = path.find_last_of("/\\");
	directory_ = (slash == std::string::npos) ? "" : path.substr(0, slash);

	globalInverseTransform_ = glm::inverse(toGlm(scene->mRootNode->mTransformation));

	// NOTE: 後で消す
	const glm::mat4 rootT = toGlm(scene->mRootNode->mTransformation);
	std::cout << "[" << path << "]root=" << scene->mRootNode->mName.C_Str() << std::endl;
	for (int r = 0; r < 4; ++r) {
	    std::cout << " " << rootT[0][r] << " " << rootT[1][r] << " " << rootT[2][r] << " " << rootT[3][r] << std::endl;
	}
	root_ = processNode(scene->mRootNode, scene);
	std::cout << " bones=" << bones_.size() << std::endl;

	// NOTE: 後で消す
	const auto boneIt = bones_.find("Bone");
	if (boneIt != bones_.end()) {
    	const glm::mat4& off = boneIt->second.offset;
        std::cout << "offset[Bone] index = " << boneIt->second.index << std::endl;
        for (int r = 0; r < 4; ++r) {
            std::cout << " " << off[0][r] << " " << off[1][r] << " " << off[2][r] << " " << off[3][r] << std::endl;
        }
	}
}

ModelNode Model::processNode(const aiNode* node, const aiScene* scene)
{
	ModelNode result;
	result.name = node->mName.C_Str();
	result.localTransform = toGlm(node->mTransformation);

	for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
		const unsigned int aiIndex = node->mMeshes[i];
		const auto found = meshIndexByAiIndex_.find(aiIndex);
		if (found != meshIndexByAiIndex_.end()) {
			result.meshIndices.push_back(found->second);
			continue;
		}
		const unsigned int ownIndex = static_cast<unsigned int>(meshes_.size());
		meshes_.push_back(processMesh(scene->mMeshes[aiIndex], scene));
		meshIndexByAiIndex_[aiIndex] = ownIndex;
		result.meshIndices.push_back(ownIndex);
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		result.children.push_back(processNode(node->mChildren[i], scene));
	}
	return result;
}

Mesh Model::processMesh(const aiMesh* mesh, const aiScene* scene)
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
		if (mesh->mTextureCoords[0])
		{
			vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertex.uv = glm::vec2(0.0f, 0.0f);
		}
		// UV を持たないメッシュでは CalcTangentSpace が接空間を作れず、零ベクトルのままになる
		if (mesh->HasTangentsAndBitangents())
		{
			vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
			vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
		}
		vertices.push_back(vertex);
	}

	std::vector<unsigned int> indices;
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	loadBones(mesh, vertices);

	gl::PbrMaterial material = loadMaterial(scene->mMaterials[mesh->mMaterialIndex], scene);
	return Mesh(std::move(vertices), std::move(indices), std::move(material));
}

void Model::loadBones(const aiMesh* mesh, std::vector<gl::Vertex>& vertices)
{
	for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
		const aiBone* bone = mesh->mBones[i];
		const std::string name = bone->mName.C_Str();

		// 同じボーンが複数のメッシュに現れるので、モデル全体で通し番号を振る
		const auto inserted = bones_.try_emplace(name);
		BoneInfo& info = inserted.first->second;
		if (inserted.second) {
			info.index = static_cast<int>(bones_.size()) - 1;
			info.offset = toGlm(bone->mOffsetMatrix);
		}

		for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
			const aiVertexWeight& weight = bone->mWeights[w];
			if (weight.mVertexId < vertices.size()) {
				addBoneInfluence(vertices[weight.mVertexId], info.index, weight.mWeight);
			}
		}
	}
}

gl::PbrMaterial Model::loadMaterial(const aiMaterial* mat, const aiScene* scene)
{
	gl::PbrMaterial material;

	material.baseColorMap = loadTexture(mat, aiTextureType_BASE_COLOR, ColorSpace::SRGB, scene);
	if (!material.baseColorMap) {
		// obj / mtl のような glTF 以前の形式向けのフォールバック
		material.baseColorMap = loadTexture(mat, aiTextureType_DIFFUSE, ColorSpace::SRGB, scene);
	}

	// metallic と roughness は同じ1枚を指す。Assimp の版によってどの型で返るかが違う
	material.metallicRoughnessMap = loadTexture(mat, aiTextureType_GLTF_METALLIC_ROUGHNESS, ColorSpace::Linear, scene);
	if (!material.metallicRoughnessMap) {
		material.metallicRoughnessMap = loadTexture(mat, aiTextureType_METALNESS, ColorSpace::Linear, scene);
	}
	if (!material.metallicRoughnessMap) {
		material.metallicRoughnessMap = loadTexture(mat, aiTextureType_DIFFUSE_ROUGHNESS, ColorSpace::Linear, scene);
	}

	material.normalMap = loadTexture(mat, aiTextureType_NORMALS, ColorSpace::Linear, scene);
	if (!material.normalMap) {
		// obj は法線マップを HEIGHT として持つことがある
		material.normalMap = loadTexture(mat, aiTextureType_HEIGHT, ColorSpace::Linear, scene);
	}

	material.occlusionMap = loadTexture(mat, aiTextureType_AMBIENT_OCCLUSION, ColorSpace::Linear, scene);
	if (!material.occlusionMap) {
		material.occlusionMap = loadTexture(mat, aiTextureType_LIGHTMAP, ColorSpace::Linear, scene);
	}

	material.emissiveMap = loadTexture(mat, aiTextureType_EMISSIVE, ColorSpace::SRGB, scene);

	aiColor4D baseColor;
	if (mat->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
		material.baseColorFactor = { baseColor.r, baseColor.g, baseColor.b };
	}
	float factor = 0.0f;
	if (mat->Get(AI_MATKEY_METALLIC_FACTOR, factor) == AI_SUCCESS) {
		material.metallic = factor;
	}
	if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, factor) == AI_SUCCESS) {
		material.roughness = factor;
	}
	aiColor3D emissive;
	if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
		material.emissiveFactor = { emissive.r, emissive.g, emissive.b };
	}

	return material;
}

std::shared_ptr<Texture> Model::loadTexture(const aiMaterial* mat, aiTextureType type, ColorSpace colorSpace,
                                            const aiScene* scene)
{
	if (mat->GetTextureCount(type) == 0) {
		return nullptr;
	}

	aiString reference;
	mat->GetTexture(type, 0, &reference);

	// glb はテクスチャの実体がファイルではなく scene->mTextures にあり、パスは "*0" のような参照になる
	if (const aiTexture* embedded = scene->GetEmbeddedTexture(reference.C_Str())) {
		if (embedded->mHeight != 0) {
			// 非圧縮の生ピクセル。glb では出てこないので未対応にしておく
			std::cerr << "Unsupported uncompressed embedded texture: " << path_ << " " << reference.C_Str()
			          << std::endl;
			return nullptr;
		}
		// mHeight == 0 のとき mWidth はピクセル数ではなくバイト数
		return cache_.getEmbedded(path_ + "|" + reference.C_Str(),
		                          reinterpret_cast<const unsigned char*>(embedded->pcData),
		                          static_cast<int>(embedded->mWidth), false, colorSpace);
	}

	return cache_.get(directory_ + "/" + reference.C_Str(), false, colorSpace);
}

void Model::Draw(gl::Shader& shader, const glm::mat4& modelMatrix) const
{
	if (!bones_.empty()) {
       shader.setMat4("model", modelMatrix * root_.localTransform);
       for (const Mesh& mesh : meshes_) {
           mesh.Draw(shader);
       }
       return;
	}
	drawNode(root_, modelMatrix, shader);
}

void Model::drawNode(const ModelNode& node, const glm::mat4& parentTransform, gl::Shader& shader) const
{
	const glm::mat4 worldTransform = parentTransform * node.localTransform;

	if (!node.meshIndices.empty()) {
		shader.setMat4("model", worldTransform);
		for (const unsigned int index : node.meshIndices) {
			meshes_[index].Draw(shader);
		}
	}

	for (const ModelNode& child : node.children) {
		drawNode(child, worldTransform, shader);
	}
}

void Model::UpdateBonePalette() {
    if (bones_.empty()) return;
    palette_.assign(bones_.size(), glm::mat4(1.0f));
    accumulatePalette(root_, glm::mat4(1.0f));
}

void Model::accumulatePalette(const ModelNode& node, const glm::mat4& parentTransform) {
    const glm::mat4 globalTransform = parentTransform * node.localTransform;

    const auto found = bones_.find(node.name);
    if(found != bones_.end()) {
        const BoneInfo& info = found->second;
        palette_[info.index] = globalInverseTransform_ * globalTransform * info.offset;
    }

    for (const ModelNode& child : node.children){
        accumulatePalette(child, globalTransform);
    }
}

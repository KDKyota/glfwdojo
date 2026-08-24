#include "Model.h"

#include <assimp/anim.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <utility>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {

// aiMatrix4x4 は行優先、glm::mat4 は列優先なので転置しながら詰め替える
glm::mat4 toGlm(const aiMatrix4x4 &m) {
    return glm::mat4(m.a1, m.b1, m.c1, m.d1,
                     m.a2, m.b2, m.c2, m.d2,
                     m.a3, m.b3, m.c3, m.d3,
                     m.a4, m.b4, m.c4, m.d4);
}

/// 空いているボーンスロットへ影響を1件追加する。
void addBoneInfluence(gl::Vertex &vertex, int boneIndex, float weight) {
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

// time をはさむ前側のキーの添え字とその区間内での補間率を返す
template <typename T>
std::pair<size_t, float> findSegment(const std::vector<AnimationKey<T>> &keys, float time) {
    // 前後のキーを見て、 キーを判断する
    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (time < keys[i + 1].time) {
            const float span = keys[i + 1].time - keys[i].time;
            return {i, span > 0.0f ? (time - keys[i].time) / span : 0.0f};
        }
    }
    return {keys.size() - 1, 0.0f};
}

// アニメーションによる位置とスケールの補完
glm::vec3 sampleVec3(const std::vector<AnimationKey<glm::vec3>> &keys, float time, const glm::vec3 &fallback) {
    if (keys.empty()) return fallback;
    // 最初のキーは 0 tick とは限らないので、範囲外は端の値で固定
    if (time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    const auto [index, factor] = findSegment(keys, time);
    return glm::mix(keys[index].value, keys[index + 1].value, factor);
}

// アニメーションによる回転の補完
glm::quat sampleQuat(const std::vector<AnimationKey<glm::quat>> &keys, float time) {
    if (keys.empty())
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (time <= keys.front().time)
        return keys.front().value;
    if (time >= keys.back().time)
        return keys.back().value;

    const auto [index, factor] = findSegment(keys, time);
    // 線形補間だと単位長からずれて回転速度がムラになるので slerp を使う
    return glm::slerp(keys[index].value, keys[index + 1].value, factor);
}
} // namespace

Model::Model(const std::string &path, TextureCache &cache) : path_(path), cache_(cache) {
    loadModel(path);
}

void Model::loadModel(const std::string &path) {
    Assimp::Importer importer;
    // LimitBoneWeights は1頂点あたりの影響を4本へ切り詰める gl::Vertex の枠と一致させるため必須
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs |
                                                       aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals |
                                                       aiProcess_LimitBoneWeights);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("ERROR::ASSIMP::" + std::string(importer.GetErrorString()));
    }
    const size_t slash = path.find_last_of("/\\");
    directory_ = (slash == std::string::npos) ? "" : path.substr(0, slash);

    globalInverseTransform_ = glm::inverse(toGlm(scene->mRootNode->mTransformation));
    root_ = processNode(scene->mRootNode, scene);

    if (static_cast<int>(bones_.size()) > kMaxBones)
        throw std::runtime_error("Too many bones: " + path + " (" + std::to_string(bones_.size()) + ") ");

    loadAnimations(scene);
    boneMatrices_.assign(bones_.size(), glm::mat4(1.0f));
    updateBoneMatrices(root_, glm::mat4(1.0f), 0.0f);

    if (!boneMatrices_.empty()) {
        boneUBO_.create();
        glBindBuffer(GL_UNIFORM_BUFFER, boneUBO_);
        // シェーダーの配列長は MAX_BONES 固定なので、実際のボーン数に関わらず枠ぶん確保する
        glBufferData(GL_UNIFORM_BUFFER, kMaxBones * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        uploadBoneMatrices();
    }
}

/// boneMatrices_ を UBO へ書き込む。
void Model::uploadBoneMatrices() {
    glBindBuffer(GL_UNIFORM_BUFFER, boneUBO_);
    // std140 の mat4 配列は 64 バイト刻みで、glm::mat4 の並びとそのまま一致する
    glBufferSubData(GL_UNIFORM_BUFFER, 0, boneMatrices_.size() * sizeof(glm::mat4), boneMatrices_.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

/// aiNode を ModelNode へ変換し、子ノードを再帰的に処理する。 その際、同じノードを二回以上送らないように  meshIndexByAiIndex_ で管理
ModelNode Model::processNode(const aiNode *node, const aiScene *scene) {
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

/// aiMesh から頂点・インデックス・マテリアルを組み立て、Mesh を作る。
Mesh Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
    std::vector<gl::Vertex> vertices;
    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        gl::Vertex vertex;
        vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

        if (mesh->HasNormals()) {
            vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
        }
        if (mesh->mTextureCoords[0]) {
            vertex.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        } else {
            vertex.uv = glm::vec2(0.0f, 0.0f);
        }
        // UV を持たないメッシュでは CalcTangentSpace が接空間を作れず、零ベクトルのままになる
        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
            vertex.bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
        }
        vertices.push_back(vertex);
    }

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace &face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    loadBones(mesh, vertices);

    gl::PbrMaterial material = loadMaterial(scene->mMaterials[mesh->mMaterialIndex], scene);
    return Mesh(std::move(vertices), std::move(indices), std::move(material), mesh->mNumBones > 0);
}

/// ボーン情報を集め、各頂点にボーンの影響を書き込む。
void Model::loadBones(const aiMesh *mesh, std::vector<gl::Vertex> &vertices) {
    for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
        const aiBone *bone = mesh->mBones[i];
        const std::string name = bone->mName.C_Str();

        // 同じボーンが複数のメッシュに現れるので、モデル全体で通し番号を振る
        const auto inserted = bones_.try_emplace(name);
        BoneInfo &info = inserted.first->second;
        if (inserted.second) {
            info.index = static_cast<int>(bones_.size()) - 1;
            info.offset = toGlm(bone->mOffsetMatrix);
            // mOfsetMatrix: そのボーンがバインドポーズでどこにあったかの逆行列
            // 動くときに関節（ジョイントを）原点として考える変換
        }

        for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
            const aiVertexWeight &weight = bone->mWeights[w];
            if (weight.mVertexId < vertices.size()) {
                addBoneInfluence(vertices[weight.mVertexId], info.index, weight.mWeight);
            }
        }
    }
}

/// aiMaterial から PBR テクスチャと factor を読み取る。
gl::PbrMaterial Model::loadMaterial(const aiMaterial *mat, const aiScene *scene) {
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
        material.baseColorFactor = {baseColor.r, baseColor.g, baseColor.b};
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
        material.emissiveFactor = {emissive.r, emissive.g, emissive.b};
    }

    return material;
}

/// type のテクスチャを TextureCache 経由でロードする。無ければ nullptr。
std::shared_ptr<Texture> Model::loadTexture(const aiMaterial *mat, aiTextureType type, ColorSpace colorSpace,
                                            const aiScene *scene) {
    if (mat->GetTextureCount(type) == 0) {
        return nullptr;
    }

    aiString reference;
    mat->GetTexture(type, 0, &reference);

    // glb はテクスチャの実体がファイルではなく scene->mTextures にあり、パスは "*0" のような参照になる
    if (const aiTexture *embedded = scene->GetEmbeddedTexture(reference.C_Str())) {
        if (embedded->mHeight != 0) {
            // 非圧縮の生ピクセル。glb では出てこないので未対応にしておく
            std::cerr << "Unsupported uncompressed embedded texture: " << path_ << " " << reference.C_Str()
                      << std::endl;
            return nullptr;
        }
        // mHeight == 0 のとき mWidth はピクセル数ではなくバイト数
        return cache_.getEmbedded(path_ + "|" + reference.C_Str(),
                                  reinterpret_cast<const unsigned char *>(embedded->pcData),
                                  static_cast<int>(embedded->mWidth), false, colorSpace);
    }

    return cache_.get(directory_ + "/" + reference.C_Str(), false, colorSpace);
}

void Model::Draw(gl::Shader &shader, const glm::mat4 &modelMatrix) const {
    if (!boneMatrices_.empty()) {
        glBindBufferBase(GL_UNIFORM_BUFFER, kBoneUBOBinding, boneUBO_);
    }
    // ボーン行列は globalInverse を掛けて root_.localTransform 分を打ち消しているので
    // スキンメッシュにはここで root_.localTransform を掛け直して辻褄を合わせる
    const glm::mat4 skinnedWorldTransform = modelMatrix * root_.localTransform;
    // 第二引数はノードの親までの累積変換 -> ルートの時点では何もたどらないのでワールド配置の modelMatrix
    drawNode(root_, modelMatrix, skinnedWorldTransform, shader);

    // 影パスのようにモデル以外と共有するシェーダーでは、hasBones を立てたまま抜けるとボーン属性を持たない VAO が既定値 aWeights=(0,0,0,1) を読んで finalBones[0] で変形される
    shader.setBool("hasBones", false);
}

void Model::drawNode(const ModelNode &node, const glm::mat4 &parentTransform, const glm::mat4 &skinnedWorldTransform, gl::Shader &shader) const {
    // updateBoneMatrices と同じ変換を辿らないと、アニメーションするノードにぶら下がる
    // 非スキンメッシュだけがバインドポーズに取り残される
    const glm::mat4 worldTransform = parentTransform * nodeTransform(node, animationTime_);

    for (const unsigned int index : node.meshIndices) {
        const Mesh &mesh = meshes_[index];
        shader.setMat4("model", mesh.IsSkinned() ? skinnedWorldTransform : worldTransform);
        shader.setBool("hasBones", mesh.IsSkinned());
        mesh.Draw(shader);
    }

    for (const ModelNode &child : node.children) {
        drawNode(child, worldTransform, skinnedWorldTransform, shader);
    }
}

/// aiAnimation をすべて読み込み、ノード名で引けるチャンネルにまとめる
void Model::loadAnimations(const aiScene *scene) {
    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        const aiAnimation *source = scene->mAnimations[i];
        Animation animation;
        animation.name = source->mName.C_Str();
        animation.duration = static_cast<float>(source->mDuration);
        if (source->mTicksPerSecond != 0.0)
            animation.ticksPerSecond = static_cast<float>(source->mTicksPerSecond);

        // すべてのチャンネル（すべてのフレーム）について操作して、ボーンの動きを取得する
        for (unsigned int c = 0; c < source->mNumChannels; ++c) {
            const aiNodeAnim *channel = source->mChannels[c];
            NodeAnimation node;

            node.positions.reserve(channel->mNumPositionKeys);
            for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k) {
                const aiVectorKey &key = channel->mPositionKeys[k];
                node.positions.push_back({static_cast<float>(key.mTime), {key.mValue.x, key.mValue.y, key.mValue.z}});
            }

            node.rotations.reserve(channel->mNumRotationKeys);
            for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k) {
                const aiQuatKey &key = channel->mRotationKeys[k];
                // glm::quat の引数順は (w, x, y, z)。aiQuaternion のメンバ並びと違う
                node.rotations.push_back({static_cast<float>(key.mTime), glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)});
            }

            node.scales.reserve(channel->mNumScalingKeys);
            for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k) {
                const aiVectorKey &key = channel->mScalingKeys[k];
                node.scales.push_back({static_cast<float>(key.mTime), {key.mValue.x, key.mValue.y, key.mValue.z}});
            }
            animation.channels[channel->mNodeName.C_Str()] = std::move(node);
        }
        animations_.push_back(std::move(animation));
    }
    if (!animations_.empty()) activeAnimation_ = 0;
}

glm::mat4 Model::nodeTransform(const ModelNode &node, float time) const {
    if (activeAnimation_ < 0) return node.localTransform;

    const Animation &animation = animations_[activeAnimation_];
    const auto found = animation.channels.find(node.name);
    if (found == animation.channels.end()) return node.localTransform;

    const NodeAnimation &channel = found->second;
    const glm::vec3 position = sampleVec3(channel.positions, time, glm::vec3(0.0f));
    const glm::quat rotation = sampleQuat(channel.rotations, time);
    const glm::vec3 scale = sampleVec3(channel.scales, time, glm::vec3(1.0f));

    return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

/// ルートには親がないので、掛けても影響がない glm::mat4(1.0f) を第二引数として渡す
void Model::updateBoneMatrices(const ModelNode &node, const glm::mat4 &parentTransform, float time) {
    const glm::mat4 globalTransform = parentTransform * nodeTransform(node, time);
    const auto found = bones_.find(node.name);
    if (found != bones_.end()) {
        const BoneInfo &info = found->second;
        // globalInverse を掛けた分、drawNode 側でスキンメッシュに root_.localTransform を掛け直して辻褄を合わせる
        boneMatrices_[info.index] = globalInverseTransform_ * globalTransform * info.offset;
    }
    for (const ModelNode &child : node.children)
        updateBoneMatrices(child, globalTransform, time);
}

void Model::UpdateAnimation(float deltaTime) {
    if (activeAnimation_ < 0 || boneMatrices_.empty()) return;

    const Animation &animation = animations_[activeAnimation_];
    animationTime_ += deltaTime * animation.ticksPerSecond;
    if (animation.duration > 0.0f)
        animationTime_ = std::fmod(animationTime_, animation.duration);

    updateBoneMatrices(root_, glm::mat4(1.0f), animationTime_);
    uploadBoneMatrices();
}

#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Shader.h"
#include "Texture.h"

namespace gl {
/**
 * @brief glTF の Metallic-Roughness ワークフローに基づく PBR マテリアル。
 */
struct PbrMaterial {
    float metallic = 0.0f;
    // 0.0 だと GGX の分布が発散するので、UI 側でも 0 まで下げないこと
    float roughness = 0.5f;

    /* ---- glTF のマテリアル テクスチャを持たないマテリアルでは factor だけが効く ---- */
    glm::vec3 baseColorFactor{1.0f};
    glm::vec3 emissiveFactor{0.0f};
    std::shared_ptr<Texture> baseColorMap;
    // metallic と roughness は glTF では1枚にパックされている（G=roughness, B=metallic）
    std::shared_ptr<Texture> metallicRoughnessMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> occlusionMap;
    std::shared_ptr<Texture> emissiveMap;

    /**
     * @brief metallic/roughness のスカラー値のみを送る。
     *
     * テクスチャを持たないマテリアル用。
     */
    void applyToShader(const Shader &shader) const {
        shader.setFloat("metallic", metallic);
        shader.setFloat("roughness", roughness);
    }

    /**
     * @brief テクスチャマップと factor をすべて送る。
     *
     * テクスチャを持つマテリアル専用。
     */
    void bindMaps(const Shader &shader) const {
        shader.setVec3("baseColorFactor", baseColorFactor);
        shader.setVec3("emissiveFactor", emissiveFactor);
        bindMap(shader, baseColorMap, "baseColorMap", "hasBaseColorMap", 0);
        bindMap(shader, metallicRoughnessMap, "metallicRoughnessMap", "hasMetallicRoughnessMap", 1);
        bindMap(shader, normalMap, "normalMap", "hasNormalMap", 2);
        bindMap(shader, occlusionMap, "occlusionMap", "hasOcclusionMap", 3);
        bindMap(shader, emissiveMap, "emissiveMap", "hasEmissiveMap", 4);
    }

  private:
    // map が無ければ hasXxxMap フラグだけ送り、シェーダー側で factor にフォールバックさせる。
    static void bindMap(const Shader &shader, const std::shared_ptr<Texture> &map, const char *sampler,
                        const char *presenceFlag, unsigned int unit) {
        shader.setBool(presenceFlag, static_cast<bool>(map));
        if (!map)
            return;
        map->bind(unit);
        shader.setInt(sampler, static_cast<int>(unit));
    }
};
} // namespace gl

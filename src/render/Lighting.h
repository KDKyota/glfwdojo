#pragma once
#include "gl/Shader.h"
#include "scene/Camera.h"
#include <cstddef>
#include <vector>
#include <glm/glm.hpp>

namespace gl {
/**
 * @brief 点光源 位置と色（ambient/diffuse/specular）と 半影の幅を決める光源半径を持つ
 *
 * constant/linear/quadratic は逆二乗減衰へ移行したため未使用
 */
struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    glm::vec3 diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 specular = glm::vec3(0.7f, 0.7f, 0.7f);
    float constant = 1.0f;
    float linear = 0.02f;
    float quadratic = 0.001f;
    float sourceRadius = 0.1f; // 半影の幅を決める光源の半径 

    // 明るさが 5/256 未満になる距離 diffuse から一意に決まる派生値なので都度計算する
    /**
     * @brief ライティング計算を打ち切ってよい距離を返す
     */
    float calcRadius() const;

    /**
     * @brief ライトの各項をシェーダーの uniform へ送る
     *
     * @param name uniform の接頭辞 配列要素なら "pointLights[0]" のように添字まで含める
     */
    void applyToShader(const Shader &shader, const std::string &name) const;
};

/**
 * @brief 平行光源（太陽光を想定） position を持たず direction のみで全シーンを一様に照らす
 */
struct DirectionalLight {
    glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    glm::vec3 diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
    glm::vec3 specular = glm::vec3(0.5f, 0.5f, 0.5f);
    /**
     * @brief ライトの各項をシェーダーの uniform へ送る
     *
     * @param name uniform の接頭辞
     */
    void applyToShader(const Shader &shader, const std::string &name) const;
};

/**
 * @brief 円錐状に照らすスポットライト cutOff/outerCutOff は照射角の cos 値
 */
struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(17.5f));
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    /**
     * @brief ライトの各項をシェーダーの uniform へ送る
     *
     * @param name uniform の接頭辞
     * @param camera position と direction を追従させるカメラ（ヘッドライト的な使い方）
     */
    void applyToShader(const Shader &shader, const std::string &name, const Camera &camera) const;
};

/**
 * @brief 点光源の配列を pointLights[i] という名前の uniform として送る
 */
void applyPointLights(const Shader &shader, const PointLight *lights, std::size_t count);

} // namespace gl

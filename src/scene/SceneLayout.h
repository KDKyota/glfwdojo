#pragma once
#include "core/SceneUnits.h"
#include "render/Lighting.h"
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

// シーン固有の配置データ 寸法は gl::units から導くこと
namespace gl::layout {

inline constexpr std::size_t kPointLightCount = 4;

inline const std::vector<glm::vec3> cubePositions = {
    glm::vec3(-1.0f, 0.0f, -1.0f),
    glm::vec3(2.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 2.5f), // 近接配置（中心間 1.2 = 隙間 0.2）
    glm::vec3(1.2f, 0.0f, 2.5f),
    glm::vec3(-1.0f, 1.0f, -1.0f), // 積み重ね
    glm::vec3(0.0f, 0.0f, -20.0f), // z=-25 壁の前
    glm::vec3(0.0f, 0.0f, 20.0f),  // z=+25 壁の前
};

/// 読み込むモデル 1 体ぶんの配置情報 読み込めなかったモデルは配置されない
struct ModelSpawn {
    std::string path;
    glm::vec3 position;
    glm::vec3 rotationDegrees{0.0f}; // 軸の向きが通常とは違うモデルを立たせるための補正
    float scale;
    bool followTarget = false; // 三人称カメラが注視するモデル
};

inline const std::vector<ModelSpawn> modelSpawns = {
    {"resources/publishable-objects/DamagedHelmet.glb", glm::vec3(-3.0f, gl::units::floorY + 1.0f, -3.0f), glm::vec3(0.0f), 1.0f},
    {"resources/characters/RiggedSimple.glb", glm::vec3(0.0f, gl::units::floorY, -3.0f), glm::vec3(0.0f), 1.0f},
    {"resources/characters/UAL1_Standard.glb", glm::vec3(3.0f, gl::units::floorY, -3.0f), glm::vec3(0.0f), 1.0f, true},
};

inline const std::array<PointLight, kPointLightCount> pointLights = {
    // constant / linear / quadratic は逆二乗減衰に移行して未使用（構造体には残置）
    {// position, ambient, diffuse, specular, constant, linear, quadratic, attenuationRadius
     {glm::vec3(0.0f, 2.0f, 2.2f), glm::vec3(0.0f), glm::vec3(20.0f, 20.0f, 20.0f),
      glm::vec3(10.0f, 10.0f, 10.0f), 1.0f, 0.14f, 0.07f, 15.0f}, // 白
     {glm::vec3(-14.5f, 1.5f, -11.0f), glm::vec3(0.0f), glm::vec3(11.0f, 2.0f, 1.25f),
      glm::vec3(5.5f, 1.0f, 0.6f), 1.0f, 0.14f, 0.07f, 20.0f}, // 赤
     {glm::vec3(4.2f, 3.0f, 1.8f), glm::vec3(0.0f), glm::vec3(2.0f, 3.0f, 11.0f), glm::vec3(1.0f, 1.5f, 5.5f),
      1.0f, 0.14f, 0.07f, 15.0f}, // 青
     {glm::vec3(-1.5f, 3.0f, -2.2f), glm::vec3(0.0f), glm::vec3(1.75f, 9.0f, 2.5f), glm::vec3(0.9f, 4.5f, 1.25f),
      1.0f, 0.14f, 0.07f, 15.0f} // 緑
    }};

// これは検証用なので後々消してもいい
inline const std::vector<glm::vec3> windowPositions = {glm::vec3(-1.5f, 0.0f, -0.48f), glm::vec3(1.5f, 0.0f, 0.51f),
                                                       glm::vec3(0.0f, 0.0f, 0.7f), glm::vec3(-0.3f, 0.0f, -2.3f),
                                                       glm::vec3(0.5f, 0.0f, -0.6f), glm::vec3(-15.0f, 0.0f, -8.0f)};

// 高さマップの配置原点
inline const glm::vec3 heightMapOrigin{gl::units::floorHalfExtent, -10.f, gl::units::heightMapExtent.z / 2 * (-1.0f)};

// 地形 1 つぶんの配置と メッシュの作り方
struct HeightMapSpawn {
    std::string filepath;
    glm::vec3 origin;
    glm::vec3 extent;
    std::uint32_t cellCountX;
    std::uint32_t cellCountZ;
    float uvTileSize; // 地形全体に UV を何周ぶん敷くか
    std::string baseColorTexturePath;
};

inline const std::vector<HeightMapSpawn> heightMapSpawns = {
    {"resources/terrain/Small-Ridge-Through-Terrain/Ridge-Through-Terrain-Height-Map.png", heightMapOrigin,
     gl::units::heightMapExtent, 1024, 1024, 1.0f,
     "resources/terrain/Small-Ridge-Through-Terrain/Ridge-Through-Terrain.png"}};

} // namespace gl::layout

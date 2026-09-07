#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "core/SceneUnits.h"

namespace gl {

// 1頂点に影響するボーンの最大数。gl::Vertex の枠と各シェーダーの配列長に一致させる
inline constexpr int kMaxBoneInfluence = 4;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    int boneIds[kMaxBoneInfluence] = {0};
    float boneWeights[kMaxBoneInfluence] = {0.0f};
};

/**
 * @brief 透明オブジェクトを奥から手前へ描くための、カメラ距離によるソート用情報。
 */
struct TransparentDraw {
    float distance;
    unsigned int index;
};

inline const std::vector<glm::vec3> skyboxVertices{
    // positions
    glm::vec3(-1.0f, 1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f, -1.0f),
    glm::vec3(1.0f, -1.0f, -1.0f),
    glm::vec3(1.0f, -1.0f, -1.0f),
    glm::vec3(1.0f, 1.0f, -1.0f),
    glm::vec3(-1.0f, 1.0f, -1.0f),

    glm::vec3(-1.0f, -1.0f, 1.0f),
    glm::vec3(-1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f, 1.0f, -1.0f),
    glm::vec3(-1.0f, 1.0f, -1.0f),
    glm::vec3(-1.0f, 1.0f, 1.0f),
    glm::vec3(-1.0f, -1.0f, 1.0f),

    glm::vec3(1.0f, -1.0f, -1.0f),
    glm::vec3(1.0f, -1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, -1.0f),
    glm::vec3(1.0f, -1.0f, -1.0f),

    glm::vec3(-1.0f, -1.0f, 1.0f),
    glm::vec3(-1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, -1.0f, 1.0f),
    glm::vec3(-1.0f, -1.0f, 1.0f),

    glm::vec3(-1.0f, 1.0f, -1.0f),
    glm::vec3(1.0f, 1.0f, -1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(-1.0f, 1.0f, 1.0f),
    glm::vec3(-1.0f, 1.0f, -1.0f),

    glm::vec3(-1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f, 1.0f),
    glm::vec3(1.0f, -1.0f, -1.0f),
    glm::vec3(1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f, 1.0f),
    glm::vec3(1.0f, -1.0f, 1.0f)};

// NDC 全面を覆うクアッド
inline const float quadVertices[] = {
    // positions   // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f};

// EBO 用に重複を除いた頂点配列（1面 = 4頂点 x 6面 = 24頂点）
inline const std::array<Vertex, 24> rawCubeVertices =
    {{
        // back face (z = -0.5)
        {{0.5f, 0.5f, -0.5f}, {}, {1.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {}, {1.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {}, {0.0f, 1.0f}},

        // front face (z = 0.5)
        {{-0.5f, -0.5f, 0.5f}, {}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {}, {0.0f, 1.0f}},

        // left face (x = -0.5)
        {{-0.5f, 0.5f, 0.5f}, {}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {}, {0.0f, 0.0f}},

        // right face (x = 0.5)
        {{0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {}, {0.0f, 0.0f}},

        // bottom face (y = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {}, {1.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {}, {1.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {}, {0.0f, 0.0f}},

        // top face (y = 0.5)
        {{-0.5f, 0.5f, -0.5f}, {}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {}, {1.0f, 1.0f}},
    }};

// 1面(4頂点)につき2つの三角形を組むためのインデックス配列(0,1,2, 2,3,0 を6面分)
inline const std::array<unsigned int, 36> cubeIndices =
    {
        0,
        1,
        2,
        2,
        3,
        0, // back
        4,
        5,
        6,
        6,
        7,
        4, // front
        8,
        9,
        10,
        10,
        11,
        8, // left
        12,
        13,
        14,
        14,
        15,
        12, // right
        16,
        17,
        18,
        18,
        19,
        16, // bottom
        20,
        21,
        22,
        22,
        23,
        20, // top
};

inline constexpr float kFloorHalf = units::floorHalfExtent;
// タイル1枚の実寸から繰り返し回数を導くので、床を広げてもテクセル密度は変わらない
inline constexpr float kFloorUv = 2.0f * units::floorHalfExtent / units::floorTileSize;

// EBO用に重複を除いた頂点配列(床)
inline const std::array<Vertex, 4> rawPlaneVertices =
    {{
        // positions // normal vectors // texture Coords
        {{-kFloorHalf, units::floorY, kFloorHalf}, {}, {0.0f, 0.0f}},
        {{kFloorHalf, units::floorY, kFloorHalf}, {}, {kFloorUv, 0.0f}},
        {{kFloorHalf, units::floorY, -kFloorHalf}, {}, {kFloorUv, kFloorUv}},
        {{-kFloorHalf, units::floorY, -kFloorHalf}, {}, {0.0f, kFloorUv}},
    }};

inline const std::array<unsigned int, 6> planeIndices = {0, 1, 2, 2, 3, 0};

// EBO用に重複を除いた頂点配列(4頂点の四角形)
inline const std::array<Vertex, 4> rawTransparentVertices =
    {{
        {{0.0f, 0.5f, 0.0f}, {}, {0.0f, 0.0f}},
        {{0.0f, -0.5f, 0.0f}, {}, {0.0f, 1.0f}},
        {{1.0f, -0.5f, 0.0f}, {}, {1.0f, 1.0f}},
        {{1.0f, 0.5f, 0.0f}, {}, {1.0f, 0.0f}},
    }};

inline const std::array<unsigned int, 6> transparentIndices = {0, 1, 2, 2, 3, 0};

glm::vec3 calcNormal(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2);

/**
 * @brief 法線マッピング用の Tangent/Bitangent を求める。
 *
 * @param v0,v1,v2 三角形の頂点座標。
 * @param uv0,uv1,uv2 対応する UV 座標。
 */
std::array<glm::vec3, 2> calcTangentBitangent(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec2 &uv0, const glm::vec2 uv1, const glm::vec2 uv2);

// 「1面 = 4頂点」の並びを前提に、面ごとに法線を計算して4頂点へ割り当てる
template <std::size_t N>
std::array<Vertex, N> calcFaceNormals(std::array<Vertex, N> vertices) {
    static_assert(N % 4 == 0, "calcFaceNormals expects 4 vertices per face");

    for (std::size_t i = 0; i < N; i += 4) {
        glm::vec3 n = calcNormal(
            vertices[i].position,
            vertices[i + 1].position,
            vertices[i + 2].position);

        vertices[i].normal =
            vertices[i + 1].normal =
                vertices[i + 2].normal =
                    vertices[i + 3].normal = n;
    }

    return vertices;
}

template <std::size_t N>
std::array<Vertex, N> calcFaceTangentBitangents(std::array<Vertex, N> vertices) {
    static_assert(N % 4 == 0, "calcFaceTangentBitangents expects 4 vertices per face");

    for (std::size_t i = 0; i < N; i += 4) {
        auto [tangent, bitangent] = calcTangentBitangent(
            vertices[i].position, vertices[i + 1].position, vertices[i + 2].position,
            vertices[i].uv, vertices[i + 1].uv, vertices[i + 2].uv);

        vertices[i].tangent =
            vertices[i + 1].tangent =
                vertices[i + 2].tangent =
                    vertices[i + 3].tangent = tangent;

        vertices[i].bitangent =
            vertices[i + 1].bitangent =
                vertices[i + 2].bitangent =
                    vertices[i + 3].bitangent = bitangent;
    }

    return vertices;
}

inline const std::array<Vertex, 24> cubeVertices = calcFaceTangentBitangents(calcFaceNormals(rawCubeVertices));
inline const std::array<Vertex, 4> planeVertices = calcFaceNormals(rawPlaneVertices);
inline const std::array<Vertex, 4> transparentVertices = calcFaceNormals(rawTransparentVertices);

inline constexpr float kWallUvU = 2.0f * units::floorHalfExtent / units::wallTileSize;
inline constexpr float kWallUvV = (units::wallTopY - units::floorY) / units::wallTileSize;

// 壁（z=-25 と z=+25 の2枚）。T×B = N が成立するよう解析的に設定
inline const std::array<Vertex, 8> wallVertices = {{
    // z=-25 の壁 (法線: +z,  T: +x, B: +y)
    {{-kFloorHalf, units::floorY, -kFloorHalf}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{kFloorHalf, units::floorY, -kFloorHalf}, {0.0f, 0.0f, 1.0f}, {kWallUvU, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{kFloorHalf, units::wallTopY, -kFloorHalf}, {0.0f, 0.0f, 1.0f}, {kWallUvU, kWallUvV}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-kFloorHalf, units::wallTopY, -kFloorHalf}, {0.0f, 0.0f, 1.0f}, {0.0f, kWallUvV}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    // z=+25 の壁 (法線: -z,  T: -x, B: +y)
    {{kFloorHalf, units::floorY, kFloorHalf}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-kFloorHalf, units::floorY, kFloorHalf}, {0.0f, 0.0f, -1.0f}, {kWallUvU, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-kFloorHalf, units::wallTopY, kFloorHalf}, {0.0f, 0.0f, -1.0f}, {kWallUvU, kWallUvV}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{kFloorHalf, units::wallTopY, kFloorHalf}, {0.0f, 0.0f, -1.0f}, {0.0f, kWallUvV}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
}};

inline const std::array<unsigned int, 12> wallIndices = {
    0,
    1,
    2,
    2,
    3,
    0, // z=-25 壁
    4,
    5,
    6,
    6,
    7,
    4, // z=+25 壁
};

} // namespace gl

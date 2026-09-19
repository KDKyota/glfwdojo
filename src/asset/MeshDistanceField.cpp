#include "asset/MeshDistanceField.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <limits>
#include <thread>
#include <vector>

namespace gl {
namespace {

// AABB の外側にも距離場を持たせる余白　extent に対する比率
// レイが面に近づく前に格子の外へ出ないための最小限の幅でよい
constexpr float kBoundsMarginRatio = 0.25f;

/// 点 p から三角形 (a, b, c) への最短距離　Ericson "Real-Time Collision Detection" 5.1.5 の実装
float pointToTriangleDistance(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
    const glm::vec3 ab = b - a;
    const glm::vec3 ac = c - a;
    const glm::vec3 ap = p - a;

    const float d1 = glm::dot(ab, ap);
    const float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
        return glm::length(p - a); // 頂点 a の外側の領域

    const glm::vec3 bp = p - b;
    const float d3 = glm::dot(ab, bp);
    const float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
        return glm::length(p - b); // 頂点 b の外側の領域

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        const float v = d1 / (d1 - d3);
        return glm::length(p - (a + v * ab)); // 辺 ab の外側の領域
    }

    const glm::vec3 cp = p - c;
    const float d5 = glm::dot(ab, cp);
    const float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
        return glm::length(p - c); // 頂点 c の外側の領域

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        const float w = d2 / (d2 - d6);
        return glm::length(p - (a + w * ac)); // 辺 ac の外側の領域
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return glm::length(p - (b + w * (c - b))); // 辺 bc の外側の領域
    }

    // 三角形の内部　重心座標で最近傍点を直接求める
    const float denom = 1.0f / (va + vb + vc);
    const float v = vb * denom;
    const float w = vc * denom;
    return glm::length(p - (a + ab * v + ac * w));
}

/// 格子点から全三角形までの符号なし最短距離
float closestUnsignedDistance(const glm::vec3 &p, const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices) {
    float minDist = std::numeric_limits<float>::max();
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const glm::vec3 &a = vertices[indices[i]].position;
        const glm::vec3 &b = vertices[indices[i + 1]].position;
        const glm::vec3 &c = vertices[indices[i + 2]].position;
        minDist = std::min(minDist, pointToTriangleDistance(p, a, b, c));
    }
    return minDist;
}

/// 一般化巻き数（Jacobson et al.）で内外を判定する　三角形が張る立体角の総和が 4π に近ければ内側
/// 奇偶則と違いメッシュが閉じていなくても破綻しない　DamagedHelmet.glb は境界エッジを 1800 本以上持つ
bool isInsideByWindingNumber(const glm::vec3 &p, const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices) {
    double solidAngleSum = 0.0;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const glm::dvec3 a = glm::dvec3(vertices[indices[i]].position) - glm::dvec3(p);
        const glm::dvec3 b = glm::dvec3(vertices[indices[i + 1]].position) - glm::dvec3(p);
        const glm::dvec3 c = glm::dvec3(vertices[indices[i + 2]].position) - glm::dvec3(p);

        const double lengthA = glm::length(a), lengthB = glm::length(b), lengthC = glm::length(c);
        const double numerator = glm::dot(a, glm::cross(b, c));
        const double denominator = lengthA * lengthB * lengthC + glm::dot(a, b) * lengthC +
                                   glm::dot(b, c) * lengthA + glm::dot(c, a) * lengthB;
        solidAngleSum += 2.0 * std::atan2(numerator, denominator);
    }
    constexpr double kFullSolidAngle = 4.0 * 3.14159265358979323846;
    return solidAngleSum / kFullSolidAngle > 0.5; // 巻き数 0.5 超で内側
}

std::vector<float> BuildSignedDistanceField(const Mesh &mesh, int resolution, const glm::vec3 &boundsMin,
                                            const glm::vec3 &boundsMax) {
    const glm::vec3 extent = boundsMax - boundsMin;
    const std::vector<Vertex> &vertices = mesh.Vertices();
    const std::vector<unsigned int> &indices = mesh.Indices();

    std::vector<float> field(static_cast<size_t>(resolution) * resolution * resolution);

    // z スライス単位で分担する　スライスごとに書き込み先が重ならないので排他は不要
    const unsigned threadCount = std::max(1u, std::min<unsigned>(std::thread::hardware_concurrency(), resolution));
    std::vector<std::thread> workers;
    workers.reserve(threadCount);
    for (unsigned worker = 0; worker < threadCount; ++worker) {
        workers.emplace_back([&, worker] {
            for (int z = static_cast<int>(worker); z < resolution; z += static_cast<int>(threadCount)) {
                for (int y = 0; y < resolution; ++y) {
                    for (int x = 0; x < resolution; ++x) {
                        // ボクセル中心のローカル座標　+0.5 は境界ではなく中心をサンプルするため
                        const glm::vec3 uvw = (glm::vec3(x, y, z) + 0.5f) / static_cast<float>(resolution);
                        const glm::vec3 p = boundsMin + uvw * extent;

                        const float unsignedDist = closestUnsignedDistance(p, vertices, indices);
                        const float sign = isInsideByWindingNumber(p, vertices, indices) ? -1.0f : 1.0f;

                        const size_t index = static_cast<size_t>(x) + y * resolution + static_cast<size_t>(z) * resolution * resolution;
                        field[index] = sign * unsignedDist;
                    }
                }
            }
        });
    }
    for (std::thread &worker : workers)
        worker.join();

    return field;
}

} // namespace

MeshDistanceField BakeMeshDistanceField(const Mesh &mesh, int resolution) {
    const glm::vec3 extent = mesh.BoundsMax() - mesh.BoundsMin();
    const glm::vec3 margin = extent * kBoundsMarginRatio;

    MeshDistanceField result;
    result.boundsMin = mesh.BoundsMin() - margin;
    result.boundsMax = mesh.BoundsMax() + margin;

    const std::vector<float> field = BuildSignedDistanceField(mesh, resolution, result.boundsMin, result.boundsMax);

    result.texture.create();
    glBindTexture(GL_TEXTURE_3D, result.texture);
    // 負の値（内側）を保持する必要があるため浮動小数点フォーマット　1チャンネルで足りる
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, resolution, resolution, resolution, 0, GL_RED, GL_FLOAT, field.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_3D, 0);

    return result;
}

} // namespace gl

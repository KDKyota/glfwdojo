#include "render/GeometryData.h"

namespace gl {

glm::vec3 calcNormal(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2) {
    return glm::normalize(glm::cross(v1 - v0, v2 - v1));
}

std::array<glm::vec3, 2> calcTangentBitangent(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec2 &uv0, const glm::vec2 uv1, const glm::vec2 uv2) {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;

    // edge = deltaUV.x * T + deltaUV.y * B という連立方程式を解くための逆行列の係数
    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    glm::vec3 tangent, bitangent;
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    tangent = glm::normalize(tangent);

    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
    bitangent = glm::normalize(bitangent);

    // 右手系になっていなければ反転する（法線マッピングの結果が反転してしまうため）
    if (glm::dot(glm::cross(tangent, bitangent), calcNormal(v0, v1, v2)) < 0.0f) {
        tangent = -tangent;
        bitangent = -bitangent;
    }

    return std::array<glm::vec3, 2>{tangent, bitangent};
}

} // namespace gl

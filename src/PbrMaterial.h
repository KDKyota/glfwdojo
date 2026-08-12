#pragma once
#include "Shader.h"

namespace gl {
struct PbrMaterial {
    float metallic = 0.0f;
    // 0.0 だと GGX の分布が発散するので、UI 側でも 0 まで下げないこと
    float roughness = 0.5f;

    // roughness は gAlbedoSpec.a を PBR 用に転用してから送る
    void applyToShader(const Shader &shader) const {
        shader.setFloat("metallic", metallic);
    }
};
} // namespace gl

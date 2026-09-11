#include "gl/NoiseTexture.h"

#include <glm/glm.hpp>
#include <random>
#include <vector>

namespace gl {

NoiseTexture::NoiseTexture() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    std::vector<glm::vec3> ssaoNoise;
    ssaoNoise.reserve(16);
    for (unsigned int i = 0; i < 16; ++i) {
        // z = 0 にするのはZ軸まわりの回転にしたいから
        ssaoNoise.emplace_back(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
    }
    texture_.create();
    glBindTexture(GL_TEXTURE_2D, texture_);
    // 負の値を保持する必要があるため浮動小数点フォーマット
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // タイル状に敷き詰めるので GL_REPEAT が必須
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

} // namespace gl

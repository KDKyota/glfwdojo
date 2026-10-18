#include "HeightField.h"
#include <stb_image.h>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace gl {

bool HeightField::LoadFromFile(const std::string &filepath) {
    int height = 0, width = 0, nChannels = 0;
    stbi_us *pixels = stbi_load_16(filepath.c_str(), &width, &height, &nChannels, 1);
    if (pixels == nullptr) {
        std::cerr << "stbi_load_16 が nullptr です" << stbi_failure_reason() << std::endl;
        stbi_image_free(pixels);
        return false;
    }
    if (height == 0 || width == 0) {
        std::cerr << "ハイトマップのファイルが正しく読み込めていません" << std::endl;
        stbi_image_free(pixels);
        return false;
    }
    heightMapDataHeight_ = static_cast<std::uint32_t>(height);
    heightMapDataWidth_ = static_cast<std::uint32_t>(width);
    heightData_.reserve(height * width);
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            stbi_us raw = pixels[z * width + x];                   // 0〜65535
            float normalized = static_cast<float>(raw) / 65535.0f; // 0〜1
            heightData_.push_back(normalized * static_cast<float>(gl::units::heightMapExtent.y));
        }
    }
    CalcResolutionLength();
    stbi_image_free(pixels);
    return true;
}

void HeightField::CalcResolutionLength() {
    resolutionLength_ = glm::vec3(static_cast<float>(gl::units::heightMapExtent.x / (heightMapDataWidth_ - 1)), 0.f,
                                  static_cast<float>(gl::units::heightMapExtent.z / (heightMapDataHeight_ - 1)));
}

float HeightField::GetHeightAt(float x, float z) const {
    float localX = x - origin_.x;
    float localZ = z - origin_.z;

    float gridCoordX = std::clamp(localX / resolutionLength_.x, 0.f, static_cast<float>(heightMapDataWidth_) - 1.f);
    float gridCoordZ = std::clamp(localZ / resolutionLength_.z, 0.f, static_cast<float>(heightMapDataHeight_) - 1.f);

    std::uint32_t pixelIndexX = std::clamp(std::floor(gridCoordX), 0.f, static_cast<float>(heightMapDataWidth_) - 2.f);
    std::uint32_t pixelIndexZ = std::clamp(std::floor(gridCoordZ), 0.f, static_cast<float>(heightMapDataHeight_) - 2.f);

    float bilinearWeightX = gridCoordX - static_cast<float>(pixelIndexX);
    float bilinearWeightZ = gridCoordZ - static_cast<float>(pixelIndexZ);

    // 4近傍の読み込み
    std::uint32_t topleft = pixelIndexZ * heightMapDataWidth_ + pixelIndexX;
    std::uint32_t topright = topleft + 1;
    std::uint32_t bottomleft = topleft + heightMapDataWidth_;
    std::uint32_t bottomright = bottomleft + 1;

    // 4 近傍のバイリニア補完
    float topValue = glm::mix(heightData_[topleft], heightData_[topright], bilinearWeightX);
    float bottomValue = glm::mix(heightData_[bottomleft], heightData_[bottomright], bilinearWeightX);
    return glm::mix(topValue, bottomValue, bilinearWeightZ);
}

} // namespace gl

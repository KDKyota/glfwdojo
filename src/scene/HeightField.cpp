#include "HeightField.h"
#include <stb_image.h>
#include <cstdint>
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

    stbi_image_free(pixels);
    return true;
}
} // namespace gl

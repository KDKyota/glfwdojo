#pragma once
#include "gl/GlHandle.h"

namespace gl {

/**
 * @brief IblBaker が起動時に焼いた IBL 用のテクスチャ一式
 */
struct IblMaps {
    TextureHandle envCubemap; // 正距円筒図法の HDR を6面へ焼き直したもの 背景と IBL の共通ソース
    TextureHandle irradianceMap;
    // ミップの各レベルが roughness 0.0 / 0.25 / 0.5 / 0.75 / 1.0 に対応する
    TextureHandle prefilterMap;
    // 環境にも材質の色にも依存しない普遍的な表
    TextureHandle brdfLut;
};

} // namespace gl

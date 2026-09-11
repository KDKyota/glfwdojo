#pragma once
#include "render/SceneGeometry.h"
#include "render/ibl/IblMaps.h"

namespace gl {

/**
 * @brief 正距円筒図法の HDR を読み込み IBL 用のテクスチャを焼いて返す
 *
 * @param viewport 焼き終えたあとに戻す画面解像度
 */
IblMaps BakeIblMaps(const SceneGeometry &geometry, int viewportWidth, int viewportHeight);

} // namespace gl

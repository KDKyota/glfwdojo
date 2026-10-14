#pragma once
#include "render/SceneGeometry.h"
#include "render/ibl/IblMaps.h"

namespace gl {

/**
 * @brief 正距円筒図法の HDR を読み込み IBL 用のテクスチャを焼いて返す
 *
 * @param geometry 焼き込みでスカイボックス用の立方体と全画面クアッドを描くために使う
 * @param viewportWidth,viewportHeight 焼き終えたあとに戻す画面解像度
 */
IblMaps BakeIblMaps(const SceneGeometry &geometry, int viewportWidth, int viewportHeight);

} // namespace gl

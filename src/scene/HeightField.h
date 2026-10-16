#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include "core/SceneUnits.h"

namespace gl {
class HeightField {
  public:
    /// ハイトマップの読み込みをして、失敗したらエラー出力
    HeightField();
    /// ワールド空間の x, z 座標からハイトマップのワールドの高さを返す関数
    float GetHeightAt(float x, float z) const;
    /// ワールド空間の x, z 座標から法線ベクトルを返す関数
    glm::vec3 GetNormalAt(float x, float z) const;
    /// resolutionLength の値を返す
    glm::vec3 GetResolutionLength() { return resolutionLength_; }
    void SetOrigin(const glm::vec3 &origin) { origin_ = origin; }
    glm::vec3 GetOrigin() const { return origin_; }
    /// PNG 画像から高さデータを読み込む
    bool LoadFromFile(const std::string &filePath);
    // resolutionLength を計算する
    glm::vec3 CalcResolutionLength();

  private:
    // 座標ごとの高さマップの情報 単位：m
    std::vector<float> heightData_{};
    // 高さマップの column のデータ数（画像の読み込み時にこの変数に値を代入する）
    std::uint32_t heightMapDataWidth_ = 0; // x軸方向のサイズ
    std::uint32_t heightMapDataHeight_ = 0; // z 軸方向のサイズ
    // 高さマップの格子一つの一辺の大きさ（基本的には y は 0として xz の辺の大きさを格納する）
    glm::vec3 resolutionLength_{};
    // 地形マップの原点
    glm::vec3 origin_{};
};
} // namespace gl

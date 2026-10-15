#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include "core/SceneUnits.h"

class HeightField {
  public:
    /// x, z 座標からハイトマップのワールドの高さを返す関数
    float GetHeightAt(float x, float z) const;
    /// 法線ベクトルを返す関数
    glm::vec3 GetNormalAt(float x, float z) const;
    /// PNG 画像から高さデータを読み込む
    bool LoadFromFile(const std::string &filePath);
    // resolutionLength を計算する
    float CalcResolutionlength();

  private:
    // 座標ごとの高さマップの情報 単位：m
    std::vector<float> heightData{};
    // 高さマップの column のデータ数（画像の読み込み時にこの変数に値を代入する）
    // 高さマップの格子一つの一辺の大きさ（縦横の大きさが等しいことを前提とする）
    float resolutionLength = 0;
};

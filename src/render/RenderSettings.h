#pragma once
#include "render/PbrMaterial.h"

namespace gl {

/**
 * @brief UI から実行時に変更する描画設定とマテリアル 毎フレームシェーダーへ送る
 */
struct RenderSettings {
    // 対応表は main.cpp の kDebugModes
    int debugMode = 0;
    // Bloom・トーンマッピング・ガンマ補正を飛ばす デバッグ表示には必須
    bool debugRawOutput = false;
    bool debugCheckerFloor = false;
    bool debugCheckerInvert = false;
    // 衝突判定の円柱をワイヤーフレームで重ねて描く
    bool debugCollision = false;
    // SSAO の効き具合（0.0 = 無効, 1.0 = そのまま適用）
    float ssaoStrength = 1.0f;
    // SDF による中距離遮蔽の聞き具合
    float sdfOcclusionStrength = 1.0f;
    // SDF ソフトシャドウの聞き具合
    float sdfShadowStrength = 1.0f;
    bool shadowMapStaticCasters = false; // 床とキューブと壁をシャドウマップにも描くか
    // SDF レイマーチの検証用
    float sdfDebugAzimuthDegrees = 0.0f;
    float sdfDebugElevationDegrees = 45.0f;

    float ambientStrength = 0.18f;    // SSAO が掛かるのはこの項だけ
    float directLightStrength = 1.0f; // IBL と直接光を切り分けて確認するための係数
    float bloomStrength = 1.0f;
    float exposure = 2.0f; // HDR 値自体は変えないので Bloom の閾値や AO のコントラストに影響しない

    /* PBR マテリアル（G-Buffer を書く4種類のオブジェクトに対応） */
    PbrMaterial cubeMaterial;
    PbrMaterial floorMaterial;
    PbrMaterial wallMaterial;
    PbrMaterial windowMaterial;
    // ガラスは誘電体なので metallic は 0 のまま（UI にも出さない）
    PbrMaterial glassMaterial;
};

} // namespace gl

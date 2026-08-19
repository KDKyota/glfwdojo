#pragma once

// このプロジェクトのワールド座標は 1.0f = 1 メートル
// glTF の 3D モデルを用いる事が前提
// glTF が既定でメートル・Y-up なので、読み込んだモデルを無変換で置けるようこちらを合わせている
// 寸法をコード中に直接書かず、必ずここの定数から導くこと
namespace gl::units {

/* ---- 人体スケールの基準値 地形やアニメーションの寸法はここから決める ---- */
inline constexpr float characterHeight = 1.7f;
inline constexpr float walkSpeed = 1.4f;
inline constexpr float runSpeed = 4.5f;
// 蹴上げ・踏面。住宅の階段に近い値
inline constexpr float stairRiser = 0.18f;
inline constexpr float stairTread = 0.27f;

// 自由視点カメラは移動が目的なので走りより速くしている
inline constexpr float freeCameraSpeed = 5.0f;

/* ---- シーンの寸法 ---- */
inline constexpr float floorHalfExtent = 25.0f;
inline constexpr float floorY = -0.5f;
// 階段で上階へ上がることを想定しているため、住宅の天井高ではなく吹き抜けの高さにしている
inline constexpr float wallTopY = 10.0f;

// テクセル密度を寸法から独立させるため、繰り返し回数ではなく「1タイルあたりの長さ」で持つ
inline constexpr float floorTileSize = 1.0f;
inline constexpr float wallTileSize = 2.0f;

} // namespace gl::units

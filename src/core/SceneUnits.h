#pragma once

// このプロジェクトのワールド座標は 1.0f = 1 メートル
// glTF が既定でメートル・Y-up なので 読み込んだモデルを無変換で置けるようこちらを合わせている
// 寸法をコード中に直接書かず 必ずここの定数から導くこと
namespace gl::units {

/* ---- 人体スケールの基準値 地形やアニメーションの寸法はここから決める ---- */
inline constexpr float characterHeight = 1.7f;
inline constexpr float walkSpeed = 1.4f;
inline constexpr float runSpeed = 5.0f;
inline constexpr float jumpSpeed = 5.0f; // ジャンプの初速（m/s）
// 蹴上げ・踏面 住宅の階段に近い値（TODO: 不自然に見える場合は調整）
inline constexpr float stairRiser = 0.18f;
inline constexpr float stairTread = 0.27f;

inline constexpr float gravity = 9.81f; // 重力加速度(m/s^2)

inline constexpr float freeCameraSpeed = 5.0f;

/* ---- シーンの寸法 ---- */
inline constexpr float floorHalfExtent = 25.0f;
inline constexpr float floorY = -0.5f;
inline constexpr float wallTopY = 10.0f;

// テクセル密度を寸法から独立させるため 繰り返し回数ではなく「1タイルあたりの長さ」で持つ
inline constexpr float floorTileSize = 1.0f;
inline constexpr float wallTileSize = 2.0f;

} // namespace gl::units

#pragma once

// Point Shadow の解像度と深度の正規化範囲
// ShadowCubeTargets / ShadowPass / DeferredLightingPass / ForwardPass が同じ値を共有する
namespace gl::shadow {

// depthCubemap_ 各面の解像度
inline constexpr unsigned int kMapWidth = 1024, kMapHeight = 1024;
// 深度は実距離を farPlane で正規化して書くので near を小さくしても精度は落ちない
inline constexpr float kNearPlane = 0.1f;
inline constexpr float kFarPlane = 50.0f; // シェーダー側の farPlane uniform と一致させる

} // namespace gl::shadow

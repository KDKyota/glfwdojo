#pragma once

// シェーダーの sampler uniform とバインド側が一致していないと 黙って真っ黒や真っ白になる
/*
 *
 * [Deferred Lighting と 透過ガラスが共有]
 *   0      gPosition
 *   1      gNormal
 *   2      gAlbedoRoughness
 *   3      shadowMap（キューブマップ配列 全光源ぶん）
 *   7      ssao
 *   8      shadowColor（キューブマップ配列 全光源ぶん）
 *   12     irradianceMap（キューブマップ）
 *   13     prefilterMap（キューブマップ）
 *   14     brdfLUT
 *   15     sdfOcclusion
 *   16..17 modelDistanceFields[2]（3D）現状は 16 だけが使われ 17 は空き
 *   20     reflectionColor
 *   21     reflectionGPosition
 *   22     reflectionGNormal
 *
 * [G-Buffer を書くパスのマテリアル]
 *   0      diffuseMap
 *   1      normalMap
 *   2      heightMap
 *
 * [SSAO パスと SDF 遮蔽パスが共有]
 *   2      noise
 *
 * [SDF 遮蔽パス専用]
 *   0..1   gPosition / gNormal（G-Buffer と同じ番号）
 *   3      gAlbedoRoughness（2 は noise が使うので G-Buffer の番号とは別）
 *
 * [画面への合成]
 *   0      screenTexture
 *   1      bloomBlur
 */
namespace gl::texunit {

/* ---- Deferred Lighting と 透過ガラスが共有する割り当て ---- */
// 注意: Deferred Lighting が同時に使う sampler の数は macOS の上限 16 個以内に収める
inline constexpr int kGPosition = 0;
inline constexpr int kGNormal = 1;
inline constexpr int kGAlbedoRoughness = 2;
inline constexpr int kShadowMap = 3;
inline constexpr int kSsao = 7;
inline constexpr int kShadowColor = 8;
inline constexpr int kIrradianceMap = 12;
inline constexpr int kPrefilterMap = 13;
inline constexpr int kBrdfLut = 14;
inline constexpr int kSdfOcclusion = 15;
// sdf_common.glsl の SDF_MAX_MODELS と一致させること
inline constexpr int kSdfMaxModels = 2;
// 静的メッシュ距離場が使う連番（kSdfModelBase 〜 kSdfModelBase + kSdfMaxModels - 1）の先頭
inline constexpr int kSdfModelBase = 16;
// 床の平面反射 SDF の枠を 4 まで戻しても重ならない番号に置く
inline constexpr int kReflectionColor = 20;
inline constexpr int kReflectionGPosition = 21;
inline constexpr int kReflectionGNormal = 22;

/* ---- G-Buffer を書くパスのマテリアル 上とは別の文脈の割り当て ---- */
inline constexpr int kDiffuseMap = 0;
inline constexpr int kNormalMap = 1;
inline constexpr int kHeightMap = 2;

/* ---- 遮蔽パスが敷くノイズ ---- */
inline constexpr int kNoise = 2;
// SDF 遮蔽パスは kNoise が 2 を使うので 粗さを読む G-Buffer だけ別の番号に置く
inline constexpr int kSdfGAlbedoRoughness = 3;

/* ---- 画面への合成 ---- */
inline constexpr int kScreenTexture = 0;
inline constexpr int kBloomBlur = 1;

} // namespace gl::texunit

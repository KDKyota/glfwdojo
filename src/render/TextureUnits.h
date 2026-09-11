#pragma once

// シェーダーの sampler uniform とバインド側が一致していないと 黙って真っ黒や真っ白になる
namespace gl::texunit {

/* ---- Deferred Lighting と 透過ガラスが共有する割り当て ---- */
inline constexpr int kGPosition = 0;
inline constexpr int kGNormal = 1;
inline constexpr int kGAlbedoRoughness = 2;
// 4灯ぶんの連番の先頭
inline constexpr int kShadowMap = 3;
// 既存の割り当て（0〜2=G-Buffer, 3〜6=shadowMap）を壊さないよう 7 を使う
inline constexpr int kSsao = 7;
// 4灯ぶんの連番の先頭
inline constexpr int kShadowColor = 8;
inline constexpr int kIrradianceMap = 12;
inline constexpr int kPrefilterMap = 13;
inline constexpr int kBrdfLut = 14;
inline constexpr int kSdfOcclusion = 15;

/* ---- G-Buffer を書くパスのマテリアル 上とは別の文脈の割り当て ---- */
inline constexpr int kDiffuseMap = 0;
inline constexpr int kNormalMap = 1;
inline constexpr int kHeightMap = 2;

/* ---- 遮蔽パスが敷くノイズ ---- */
inline constexpr int kNoise = 2;

/* ---- 画面への合成 ---- */
inline constexpr int kScreenTexture = 0;
inline constexpr int kBloomBlur = 1;

} // namespace gl::texunit

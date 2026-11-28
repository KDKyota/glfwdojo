#pragma once
// MacOS に対応させるときに 4.1 まででは　GLSL の binding が使えないので ここでリンクした直後に番号を割り当てることにする
#include <glad/glad.h>

/// UBO のブロック名と binding 番号の対応 Shader と glBindBufferBase の両方がここを参照する
namespace gl::ubo {

// 以下の三つは今まで binding で使っていたバインディング番号を定数化したもの
/// 全パスが view と projection を読む Matrices ブロックの binding
inline constexpr GLuint kMatricesBinding = 0;
/// スキニングで使う BoneMatrices ブロックの binding
inline constexpr GLuint kBoneMatricesBinding = 1;
/// SDF オクルージョンが読む SdfScene ブロックの binding
inline constexpr GLuint kSdfSceneBinding = 2;

/// GLSL 側のブロック名と binding 番号の組
struct BlockBinding {
    const char *blockName;
    GLuint binding;
};

/// 注意: blockName はシェーダーの uniform ブロック名と完全に一致させること ブロックを追加したいときはここに追加すること
inline constexpr BlockBinding kBlockBindings[] = {
    {"Matrices", kMatricesBinding},
    {"BoneMatrices", kBoneMatricesBinding},
    {"SdfScene", kSdfSceneBinding},
};

} // namespace gl::ubo
#pragma once
#include "gl/GlHandle.h"

namespace gl {

/// 出力先の FBO とサイズ パスを特定のターゲット型に縛らないために使う
struct TargetView {
    GLuint fbo;
    int width, height;
};

/**
 * @brief 2D テクスチャを確保して現在バインド中の FBO のアタッチメントへ繋ぐ
 *
 * @param[out] texture 確保して FBO へ繋ぐテクスチャ
 * @param attachment 繋ぎ先のアタッチメント（GL_COLOR_ATTACHMENTi）
 * @param filter ブラーを掛ける先は GL_LINEAR 位置や法線をそのまま読む先は GL_NEAREST
 */
void AttachColorTarget(TextureHandle &texture, GLenum attachment, GLint internalFormat, GLenum format, GLenum type,
                       int width, int height, GLint filter);

/**
 * @brief 現在バインド中の FBO へ深度ステンシル用のレンダーバッファを繋ぐ
 *
 * @param[out] renderbuffer 確保して FBO へ繋ぐレンダーバッファ
 */
void AttachDepthStencilBuffer(RenderbufferHandle &renderbuffer, int width, int height);

/**
 * @brief 6面ぶんの領域を確保したキューブマップを作る
 *
 * @param[out] texture 6面ぶんを確保するキューブマップ
 * @param size 1面の一辺のピクセル数
 * @param minFilter ミップを引く用途では GL_LINEAR_MIPMAP_LINEAR を渡す
 */
void CreateCubemap(TextureHandle &texture, GLint internalFormat, GLenum format, GLenum type, int size,
                   GLint minFilter, GLint magFilter);

/**
 * @brief 現在バインド中の FBO が完全かを確かめ 不完全なら name を添えて報告する
 */
void CheckFramebufferComplete(const char *name);

} // namespace gl

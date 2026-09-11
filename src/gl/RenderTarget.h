#pragma once
#include "gl/GlHandle.h"

namespace gl {

/**
 * @brief 2D テクスチャを確保して現在バインド中の FBO のアタッチメントへ繋ぐ
 *
 * @param filter ブラーを掛ける先は GL_LINEAR 位置や法線をそのまま読む先は GL_NEAREST
 */
void AttachColorTarget(TextureHandle &texture, GLenum attachment, GLint internalFormat, GLenum format, GLenum type,
                       int width, int height, GLint filter);

/**
 * @brief 現在バインド中の FBO へ深度ステンシル用のレンダーバッファを繋ぐ
 */
void AttachDepthStencilBuffer(RenderbufferHandle &renderbuffer, int width, int height);

/**
 * @brief 6面ぶんの領域を確保したキューブマップを作る
 *
 * @param minFilter ミップを引く用途では GL_LINEAR_MIPMAP_LINEAR を渡す
 */
void CreateCubemap(TextureHandle &texture, GLint internalFormat, GLenum format, GLenum type, int size,
                   GLint minFilter, GLint magFilter);

/**
 * @brief 現在バインド中の FBO が完全かを確かめ 不完全なら name を添えて報告する
 */
void CheckFramebufferComplete(const char *name);

} // namespace gl

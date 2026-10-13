#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include "gl/Texture.h"

/**
 * @brief 同じ画像を複数回ロードしないための Texture の弱参照キャッシュ
 */
class TextureCache {
  private:
    // キーはパス + 色空間 パスだけだと同じ画像を別の色空間で読んだとき取り違える
    std::unordered_map<std::string, std::weak_ptr<Texture>> cache_;

  public:
    /**
     * @brief キャッシュにあれば返し 無ければ読み込んで登録する
     *
     * @param path 画像ファイルのパス
     * @param flip 読み込み時に上下を反転するか
     * @param colorSpace 色かデータかを表す色空間
     */
    std::shared_ptr<Texture> get(const std::string &path, bool flip, ColorSpace colorSpace);

    /**
     * @brief glb の埋め込みテクスチャを キャッシュにあれば返し 無ければ読み込んで登録する
     *
     * @param key キャッシュ上でファイルパスの代わりに使う識別子 呼び出し側が一意に作り 同じ glb 内で衝突しないよう「モデルのパス + 参照名」にする
     * @param data 圧縮された画像のバイト列
     * @param byteSize data のバイト数
     * @param flip 読み込み時に上下を反転するか
     * @param colorSpace 色かデータかを表す色空間
     */
    std::shared_ptr<Texture> getEmbedded(const std::string &key, const unsigned char *data, int byteSize, bool flip, ColorSpace colorSpace);

    /**
     * @brief 6枚の画像から GL_TEXTURE_CUBE_MAP を作る
     *
     * キャッシュはしない
     * @param faces 6面ぶんの画像パス +X -X +Y -Y +Z -Z の順
     * @param flip 読み込み時に上下を反転するか
     * @param colorSpace 色かデータかを表す色空間
     * @return 作成した GL テクスチャの ID 呼び出し側が解放する
     */
    unsigned int loadCubemap(const std::vector<std::string> &faces, bool flip, ColorSpace colorSpace);
};

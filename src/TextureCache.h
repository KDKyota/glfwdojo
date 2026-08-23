#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include "Texture.h"

/**
 * @brief 同じ画像を複数回ロードしないための Texture の弱参照キャッシュ。
 */
class TextureCache {
  private:
    // キーはパス + 色空間。パスだけだと同じ画像を別の色空間で読んだとき取り違える
    std::unordered_map<std::string, std::weak_ptr<Texture>> cache_;

  public:
    /**
     * @brief キャッシュにあれば返し、無ければ読み込んで登録する。
     *
     * @param path 画像ファイルのパス。
     * @param colorSpace 色かデータかを表す色空間。
     */
    std::shared_ptr<Texture> get(const std::string &path, bool flip, ColorSpace colorSpace);

    /*
     * glb の埋め込みテクスチャ用。実体がファイルではないので、呼び出し側が一意な key を作って渡す
     * （同じ glb 内でインデックスが衝突しないよう「モデルのパス + 参照名」にする）
     */
    std::shared_ptr<Texture> getEmbedded(const std::string &key, const unsigned char *data, int byteSize, bool flip, ColorSpace colorSpace);

    /**
     * @brief 6枚の画像から GL_TEXTURE_CUBE_MAP を作る。
     *
     * キャッシュはしない。
     * @param faces 6面ぶんの画像パス。
     */
    unsigned int loadCubemap(const std::vector<std::string> &faces, bool flip, ColorSpace colorSpace);
};

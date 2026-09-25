#pragma once
#include "asset/MeshDistanceField.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace gl {

// 3D モデルの AABB の一辺における格子点の数
inline constexpr int kSdfBakeResolution = 32;

/**
 * @brief 同じメッシュの距離場を複数回焼かないための弱参照キャッシュ
 */
class MeshDistanceFieldCache {
  private:
    std::unordered_map<std::string, std::weak_ptr<const MeshDistanceField>> cache_;

  public:
    /**
     * @brief キャッシュにあれば返して、なければ焼いて登録する
     *
     * @param modelPath 識別子の一部 Mesh 自身は読み込み元のパスを知らないので呼び出し側が渡す
     * @param meshIndex モデル内のメッシュ番号
     */
    std::shared_ptr<const MeshDistanceField> get(const std::string &modelPath, unsigned int meshIndex,
                                                 const Mesh &mesh);
};

} // namespace gl

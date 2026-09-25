#include "asset/MeshDistanceFieldCache.h"
#include "asset/MeshDistanceField.h"
#include "sdf/DistanceFieldBuilder.h"
#include <iostream>

namespace gl {
namespace {

// この設定で焼いた距離場を一意に識別する文字列　一致するときだけ同じ焼き上がりとみなす
std::string makeBakeKey(const std::string &modelPath, unsigned int meshIndex) {
    return modelPath + "|" + std::to_string(meshIndex) + "|" + std::to_string(kSdfBakeResolution) + "|" +
           std::to_string(kBoundsMarginRatio);
}
} // namespace

std::shared_ptr<const MeshDistanceField> MeshDistanceFieldCache::get(const std::string &modelPath,
                                                                     unsigned int meshIndex, const Mesh &mesh) {
    const std::string key = makeBakeKey(modelPath, meshIndex);
    if (std::shared_ptr<const MeshDistanceField> cached = cache_[key].lock()) {
        std::cout << "SDF cache hit: " << key << std::endl;
        return cached;
    }
    std::cout << "SDF cache miss: " << key << std::endl;
    std::shared_ptr<const MeshDistanceField> field =
        std::make_shared<const MeshDistanceField>(BakeMeshDistanceField(mesh, kSdfBakeResolution));
    cache_[key] = field;
    return field;
}
} // namespace gl

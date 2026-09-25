#include "asset/MeshDistanceFieldCache.h"
#include "asset/MeshDistanceField.h"
#include "sdf/DistanceFieldBuilder.h"
#include <ios>
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <type_traits>
#include <optional>

namespace gl {
namespace {
// FNV-1a 非暗号化ハッシュ化のための定数
constexpr std::uint64_t kStableHashSeed = 14695981039346656037ull;
constexpr std::uint64_t kStableHashMultiplier = 1099511628211ull;

constexpr const char *kDistanceFieldCacheDirectory = "sdf_cache";
constexpr std::uint32_t kCacheFileVersion = 1;

/**
 * @brief ディスクに保存するハッシュ値を計算する
 */
std::uint64_t computeStableHash(const void *data, std::size_t size, std::uint64_t seed) {
    const unsigned char *bytes = static_cast<const unsigned char *>(data);
    std::uint64_t hash = seed;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= kStableHashMultiplier;
    }
    return hash;
}

template <typename T> void writeInBinary(std::ostream &out, const T &value) {
    static_assert(std::is_trivially_copyable_v<T>, "バイト列をそのまま書けるのは trivially copyable な型だけ");
    out.write(reinterpret_cast<const char *>(&value), sizeof(T));
}
template <typename T> void readInBinary(std::istream &in, T &value) {
    static_assert(std::is_trivially_copyable_v<T>, "バイト列を読めるのは trivially copyable な型だけ");
    in.read(reinterpret_cast<char *>(&value), sizeof(T));
}
// 距離場を決める頂点座標とインデックスを入力としてハッシュ化する
std::uint64_t hashDistanceFieldInputs(const Mesh &mesh) {
    std::uint64_t hash = kStableHashSeed;
    for (const Vertex &vertex : mesh.Vertices()) {
        hash = computeStableHash(&vertex.position, sizeof(vertex.position), hash);
    }

    const std::vector<unsigned int> &indices = mesh.Indices();
    return computeStableHash(indices.data(), indices.size() * sizeof(unsigned int), hash);
}
// この設定で焼いたモデルの距離場を一意に識別する文字列(キー)　
std::string makeBakeKey(const std::string &modelPath, unsigned int meshIndex, const Mesh &mesh) {
    return modelPath + "|" + std::to_string(meshIndex) + "|" + std::to_string(kSdfBakeResolution) + "|" +
           std::to_string(kBoundsMarginRatio) + "|" + std::to_string(hashDistanceFieldInputs(mesh));
}

std::filesystem::path getCacheFilePathFromKey(const std::string &bakeKey) {
    char fileName[32];
    // キーは / や | を含むので16進数表記をファイル名にする
    std::snprintf(fileName, sizeof(fileName), "%016llx.sdf",
                  static_cast<unsigned long long>(computeStableHash(bakeKey.data(), bakeKey.size(), kStableHashSeed)));
    return std::filesystem::path(kDistanceFieldCacheDirectory) / fileName;
}

void saveBakedDistanceField(const std::filesystem::path &cacheFile, const std::string &bakeKey,
                            const BakedDistanceField &baked) {
    std::error_code error;
    std::filesystem::create_directories(cacheFile.parent_path(), error);
    std::filesystem::path temporaryFile = cacheFile;
    temporaryFile += ".tmp";
    {
        // 注意：改行コードの変換を避けるためバイナ李で開き、書き込む
        std::ofstream out(temporaryFile, std::ios::binary);
        // 書き込む順番：バージョン、キーのデータサイズ、キー、格子数、最小値、最大値、距離場のデータ
        writeInBinary(out, kCacheFileVersion);
        writeInBinary(out, static_cast<std::uint32_t>(bakeKey.size()));
        out.write(bakeKey.data(), static_cast<std::streamsize>(bakeKey.size()));
        writeInBinary(out, baked.resolution);
        writeInBinary(out, baked.boundsMin);
        writeInBinary(out, baked.boundsMax);
        out.write(reinterpret_cast<const char *>(baked.values.data()),
                  static_cast<std::streamsize>(baked.values.size() * sizeof(float)));
        if (!out) {
            std::cerr << "Failed to write SDF cache: " << temporaryFile << std::endl;
            return;
        }
    }
    std::filesystem::rename(temporaryFile, cacheFile, error);
}

std::optional<BakedDistanceField> loadBakedDistanceField(const std::filesystem::path &cacheFile,
                                                         const std::string &bakeKey) {
    std::ifstream in(cacheFile, std::ios::binary);
    if (!in) return std::nullopt;

    std::uint32_t version = 0; // 変数を初期化しているだけ
    readInBinary(in, version);
    if (version != kCacheFileVersion) return std::nullopt;

    std::uint32_t keyLength = 0;
    readInBinary(in, keyLength);
    if (keyLength != bakeKey.size()) return std::nullopt;
    std::string storedKey(keyLength, '\0');
    in.read(storedKey.data(), keyLength);
    if (storedKey != bakeKey) return std::nullopt;

    BakedDistanceField baked;
    readInBinary(in, baked.resolution);
    readInBinary(in, baked.boundsMin);
    readInBinary(in, baked.boundsMax);
    if (!in || baked.resolution != kSdfBakeResolution) return std::nullopt;

    baked.values.resize(static_cast<std::size_t>(baked.resolution) * baked.resolution * baked.resolution);
    in.read(reinterpret_cast<char *>(baked.values.data()),
            static_cast<std::streamsize>(baked.values.size() * sizeof(float)));
    if (!in) return std::nullopt;
    return baked;
}

} // namespace

std::shared_ptr<const MeshDistanceField> MeshDistanceFieldCache::get(const std::string &modelPath,
                                                                     unsigned int meshIndex, const Mesh &mesh) {
    const std::string key = makeBakeKey(modelPath, meshIndex, mesh);
    if (std::shared_ptr<const MeshDistanceField> cached = cache_[key].lock()) {
        std::cout << "SDF cache hit: " << key << std::endl;
        return cached;
    }
    const std::filesystem::path cacheFile = getCacheFilePathFromKey(key);
    std::optional<BakedDistanceField> baked = loadBakedDistanceField(cacheFile, key);
    if (baked) {
        std::cout << "SDF disk hit: " << key << std::endl;
    } else {
        std::cout << "SDF miss: " << key << std::endl;
        baked = BakeDistanceField(mesh, kSdfBakeResolution);
        saveBakedDistanceField(cacheFile, key, *baked);
    }

    std::shared_ptr<const MeshDistanceField> field =
        std::make_shared<const MeshDistanceField>(UploadMeshDistanceField(*baked));
    cache_[key] = field;
    return field;
}

} // namespace gl

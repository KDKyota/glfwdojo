#pragma once
#include <string>

// bool を並べると flip と取り違えるため enum にしている
/**
 * @brief ピクセル値が sRGB エンコードされているか、リニアなデータかを表す。
 */
enum class ColorSpace {
    Linear,
    SRGB
};

/**
 * @brief 画像を読み込み、2D テクスチャとして GPU へアップロードするクラス。
 */
class Texture {
  private:
    unsigned int id_;
    std::string type_;
    std::string path_;
    bool flip_;
    ColorSpace colorSpace_;

    // stbi が返したピクセル列を GL へ送る。ファイル版とメモリ版で共通
    void uploadPixels(unsigned char *pixels, int width, int height, int channels);

  public:
    // colorSpace にデフォルト値を持たせないのは、色かデータかを呼び出し側に必ず選ばせるため
    Texture(const char *path, const bool flip, const ColorSpace colorSpace);
    // glb の埋め込みテクスチャ用。data はメモリ上の圧縮画像、key はファイルとして存在しない識別子
    Texture(const std::string &key, const unsigned char *data, int byteSize, const bool flip,
            const ColorSpace colorSpace);
    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    // ムーブは許可（shared_ptr 内部で利用される）
    Texture(Texture &&other) noexcept;
    Texture &operator=(Texture &&) noexcept;

    void bind(unsigned int unit) const;
    unsigned int getID() const;
};

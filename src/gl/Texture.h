#pragma once
#include <string>

// bool を並べると flip と取り違えるため enum にしている
/**
 * @brief ピクセル値が sRGB エンコードされているか リニアなデータかを表す
 */
enum class ColorSpace {
    Linear,
    SRGB
};

/**
 * @brief 画像を読み込み 2D テクスチャとして GPU へアップロードするクラス
 */
class Texture {
  private:
    unsigned int id_;
    std::string type_;
    std::string path_;
    bool flip_;
    ColorSpace colorSpace_;

    // stbi が返したピクセル列を GL へ送る ファイル版とメモリ版で共通
    void uploadPixels(unsigned char *pixels, int width, int height, int channels);

  public:
    /**
     * @brief 画像ファイルを読み込んで GPU へ送る
     *
     * colorSpace にデフォルト値を持たせないのは 色かデータかを呼び出し側に必ず選ばせるため
     *
     * @param flip 読み込み時に上下を反転するか
     * @param colorSpace 色なら SRGB データなら Linear
     */
    Texture(const char *path, const bool flip, const ColorSpace colorSpace);
    /**
     * @brief glb の埋め込みテクスチャをメモリから読み込んで GPU へ送る
     *
     * @param key キャッシュ上でファイルパスの代わりに使う識別子
     * @param data 圧縮された画像のバイト列
     * @param byteSize data のバイト数
     * @param flip 読み込み時に上下を反転するか
     * @param colorSpace 色なら SRGB データなら Linear
     */
    Texture(const std::string &key, const unsigned char *data, int byteSize, const bool flip,
            const ColorSpace colorSpace);
    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    // ムーブは許可（shared_ptr 内部で利用される）
    Texture(Texture &&other) noexcept;
    Texture &operator=(Texture &&) noexcept;

    /**
     * @brief このテクスチャを unit へバインドする
     *
     * @param unit テクスチャユニット番号（GL_TEXTURE0 からの連番）
     */
    void bind(unsigned int unit) const;
    unsigned int getId() const;
};

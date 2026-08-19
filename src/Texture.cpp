#include "Texture.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <iostream>

void Texture::uploadPixels(unsigned char* pixels, int width, int height, int channels)
{
    glBindTexture(GL_TEXTURE_2D, id_);

    const GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    if (pixels) {
        // sRGB 復号の対象は RGB のみで、アルファはリニアのまま（window.png の閾値判定が依存）
        const GLenum internalFormat = (colorSpace_ == ColorSpace::SRGB)
            ? ((channels == 4) ? GL_SRGB8_ALPHA8 : GL_SRGB8)
            : format;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture::Texture(const char* path, bool flip, ColorSpace colorSpace)
	: id_(0), path_(path), flip_(flip), colorSpace_(colorSpace)
{
    stbi_set_flip_vertically_on_load(flip);
    glGenTextures(1, &id_);

    int width = 0, height = 0, nrChannels = 3;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    uploadPixels(data, width, height, nrChannels);
    stbi_image_free(data);
}

Texture::Texture(const std::string& key, const unsigned char* data, int byteSize, bool flip, ColorSpace colorSpace)
    : id_(0), path_(key), flip_(flip), colorSpace_(colorSpace)
{
    stbi_set_flip_vertically_on_load(flip);
    glGenTextures(1, &id_);

    int width = 0, height = 0, nrChannels = 3;
    unsigned char* pixels = stbi_load_from_memory(data, byteSize, &width, &height, &nrChannels, 0);
    if (!pixels) {
        std::cerr << "Failed to decode embedded texture: " << key << std::endl;
    }
    uploadPixels(pixels, width, height, nrChannels);
    stbi_image_free(pixels);
}

Texture::~Texture()
{
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
    }
}

// ムーブコントラクタ
Texture::Texture(Texture&& other)  noexcept
    : id_(other.id_), path_(std::move(other.path_)), flip_(other.flip_), colorSpace_(other.colorSpace_)
{
    other.id_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        glDeleteTextures(1, &id_); // 自身が持っているリソースをまずは解放
        // その後値をコピー
        id_ = other.id_;
        path_ = std::move(other.path_);
        flip_ = other.flip_;
        colorSpace_ = other.colorSpace_;

        other.id_ = 0;
    }
    return *this;
}

void Texture::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

unsigned int Texture::getID() const { return id_; }

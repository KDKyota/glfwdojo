#include "Texture.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <iostream>

Texture::Texture(const char* path, bool flip)
	: id_(0), path_(path), flip_(flip)
{
    stbi_set_flip_vertically_on_load(flip);

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);


    int width, height, nrChannels;
    GLenum format;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
}

Texture::~Texture()
{
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
    }
}

// ムーブコントラクタ
Texture::Texture(Texture&& other)  noexcept
    : id_(other.id_), path_(std::move(other.path_)), flip_(other.flip_)
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

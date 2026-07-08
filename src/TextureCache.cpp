#include <glad/glad.h>   // ← 最初に GLAD（OpenGL関数の定義）
#include "TextureCache.h"
#include <stb_image.h>   // ← stbi_load に必要
#include <iostream>      // ← std::cout に必要

std::shared_ptr<Texture> TextureCache::get(const std::string& path, bool flip)
{
	if (auto tex = cache_[path].lock()) {
		return tex;
	}
	else {
		std::shared_ptr<Texture> texture = std::make_shared<Texture>(path.c_str(), flip);
		cache_[path] = texture;
		return texture;
	}
}

unsigned int TextureCache::loadCubemap(const std::vector<std::string>& faces, bool flip)
{
    stbi_set_flip_vertically_on_load(flip);
	unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}




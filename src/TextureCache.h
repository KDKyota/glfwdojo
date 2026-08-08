#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include "Texture.h"

class TextureCache {
private:
	// キーはパス + 色空間。パスだけだと同じ画像を別の色空間で読んだとき取り違える
	std::unordered_map<std::string, std::weak_ptr<Texture>> cache_;

public:
	std::shared_ptr<Texture> get(const std::string& path, bool flip, ColorSpace colorSpace);

	unsigned int loadCubemap(const std::vector<std::string>& faces, bool flip, ColorSpace colorSpace);
};
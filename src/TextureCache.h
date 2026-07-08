#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include "Texture.h"

class TextureCache {
private:
	std::unordered_map<std::string, std::weak_ptr<Texture>> cache_;

public: 
	std::shared_ptr<Texture> get(const std::string& path, bool flip = false);

	unsigned int loadCubemap(const std::vector<std::string>& faces, bool flip = false);
};
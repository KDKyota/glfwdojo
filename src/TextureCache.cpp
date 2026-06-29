#include "TextureCache.h"

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




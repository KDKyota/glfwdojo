#pragma once
#include <memory>
#include "Texture.h"
#include "Shader.h"

struct Material {
private:
	float shininess = 32.0f;

public:

	std::shared_ptr<Texture> diffuse;
	std::shared_ptr<Texture> specular;
	void bind() const;
	void setShininess(const float shi);
	void setUniforms(gl::Shader& shader) const;
};

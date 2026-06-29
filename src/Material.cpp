#include "Material.h"

void Material::bind() const
{
	if (diffuse) diffuse->bind(0);
	if (specular) specular->bind(1);
}

void Material::setShininess(const float shi)
{
	if (shi < 1) {
		throw std::runtime_error("shininessは0以上の少数");
	}
	shininess = shi;
}

void Material::setUniforms(gl::Shader& shader) const
{
	shader.setInt("material.diffuse", 0);
	shader.setInt("material.specular", 1);
	shader.setFloat("material.shininess", shininess);
}
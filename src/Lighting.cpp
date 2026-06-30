#include "Lighting.h"

namespace gl {
	void PointLight::applyToShader(const Shader& shader, const std::string& name) const {
		shader.setVec3(name + ".position", position);
		shader.setVec3(name + ".ambient", ambient);
		shader.setVec3(name + ".diffuse", diffuse);
		shader.setVec3(name + ".specular", specular);
		shader.setFloat(name + ".constant", constant);
		shader.setFloat(name + ".linear", linear);
		shader.setFloat(name + ".quadratic", quadratic);
	}

	void DirectionalLight::applyToShader(const Shader& shader, const std::string& name) const {
		shader.setVec3(name + ".direction", direction);
		shader.setVec3(name + ".ambient", ambient);
		shader.setVec3(name + ".diffuse", diffuse);
		shader.setVec3(name + ".specular", specular);
	}

	void SpotLight::applyToShader(const Shader& shader, const std::string& name, const Camera& camera) const {
		shader.setVec3(name + ".position", camera.GetViewPosition());
		shader.setVec3(name + ".direction", camera.GetViewFront());
		shader.setVec3(name + ".ambient", ambient);
		shader.setVec3(name + ".diffuse", diffuse);
		shader.setVec3(name + ".specular", specular);
		shader.setFloat(name + ".cutOff", cutOff);
		shader.setFloat(name + ".outerCutOff", outerCutOff);
		shader.setFloat(name + ".constant", constant);
		shader.setFloat(name + ".linear", linear);
		shader.setFloat(name + ".quadratic", quadratic);
	}
}

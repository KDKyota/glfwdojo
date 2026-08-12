#include "Lighting.h"

#include <algorithm>
#include <cmath>

namespace gl {
	float PointLight::calcRadius() const {
		// 最も強いチャンネルを基準にする（RGBのどれか1つでも見えていれば影響ありとみなす）
		const float lightMax = std::max({ diffuse.r, diffuse.g, diffuse.b });
		// 8bit カラーの1階調に満たない 5/256 を「もう見えない」とみなす打ち切り閾値
		const float threshold = 256.0f / 5.0f * lightMax;

		// シェーダー側の逆二乗減衰 lightMax / d^2 = 5/256 を d について解いた形
		return std::sqrt(threshold);
	}

	void PointLight::applyToShader(const Shader& shader, const std::string& name) const {
		shader.setVec3(name + ".position", position);
		shader.setVec3(name + ".ambient", ambient);
		shader.setVec3(name + ".diffuse", diffuse);
		shader.setVec3(name + ".specular", specular);
		shader.setFloat(name + ".constant", constant);
		shader.setFloat(name + ".linear", linear);
		shader.setFloat(name + ".quadratic", quadratic);
		shader.setFloat(name + ".radius", calcRadius());
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

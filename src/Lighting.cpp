#include "Lighting.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gl {
	float PointLight::calcRadius() const {
		// 最も強いチャンネルを基準にする（RGBのどれか1つでも見えていれば影響ありとみなす）
		const float lightMax = std::max({ diffuse.r, diffuse.g, diffuse.b });
		// 打ち切りの閾値。8bit カラーの1階調 = 1/256 なので、その手前の 5/256 を「もう見えない」とみなす
		const float threshold = 256.0f / 5.0f * lightMax;

		// constant + linear * d + quadratic * d^2 = threshold を d について解く
		if (quadratic > 0.0f) {
			const float discriminant = linear * linear - 4.0f * quadratic * (constant - threshold);
			// 判別式が負 = そもそも閾値に達するほど明るくない（＝どこにも届かない）
			if (discriminant < 0.0f)
				return 0.0f;
			return (-linear + std::sqrt(discriminant)) / (2.0f * quadratic);
		}
		// quadratic = 0 なら一次方程式に退化する
		if (linear > 0.0f)
			return std::max((threshold - constant) / linear, 0.0f);
		// 距離で減衰しない設定。無限遠まで届くので打ち切らない
		return std::numeric_limits<float>::max();
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

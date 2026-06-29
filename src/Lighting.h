#pragma once
#include <glm/glm.hpp>

namespace gl {
	struct Light {
		glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
		glm::vec3 position = glm::vec3(1.2f, 1.0f, 2.0f);
		glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
		glm::vec3 diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
		glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
		float ambientStrength = 0.1f;
		float specularStrength = 0.5f;
		// attenuation（距離による減衰）
		float constant = 1.0f; // これは通常固定値
		float linear = 0.09f; // 距離に比例して減衰する係数
		float quadratic = 0.032f; // 距離の二乗に比例して減衰する係数
		float cutOff = glm::cos(glm::radians(12.5f)); // スポットライトの内側の角度（ラジアン）
		float outerCutOff = glm::cos(glm::radians(17.5f)); // スポットライトの外側の角度（ラジアン）
	};
}

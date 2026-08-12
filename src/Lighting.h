#pragma once
#include "Shader.h"
#include "Camera.h"
#include <vector>
#include <glm/glm.hpp>

namespace gl {
	struct PointLight {
		glm::vec3 position ;
		glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);
		glm::vec3 diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		glm::vec3 specular = glm::vec3(0.7f, 0.7f, 0.7f);
		float constant = 1.0f;
		float linear = 0.02f;
		float quadratic = 0.001f;

		// 明るさが 5/256 未満になる距離。diffuse から一意に決まる派生値なので都度計算する
		float calcRadius() const;

		void applyToShader(const Shader& shader, const std::string& name) const;
	};

	struct DirectionalLight {
		glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
		glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);
		glm::vec3 diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
		glm::vec3 specular = glm::vec3(0.5f, 0.5f, 0.5f);
		void applyToShader(const Shader& shader, const std::string& name) const;
	};

	struct SpotLight {
		glm::vec3 position;
		glm::vec3 direction;
		glm::vec3 ambient = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
		float cutOff = glm::cos(glm::radians(12.5f));
		float outerCutOff = glm::cos(glm::radians(17.5f));
		float constant = 1.0f;
		float linear = 0.09f;
		float quadratic = 0.032f;
		void applyToShader(const Shader& shader, const std::string& name, const Camera& camera) const;
	};

}

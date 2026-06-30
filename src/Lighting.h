#pragma once
#include "Shader.h"
#include "Camera.h"
#include <vector>
#include <glm/glm.hpp>

namespace gl {
	struct PointLight {
		glm::vec3 position ;
		glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);
		glm::vec3 diffuse = glm::vec3(0.3f, 0.3f, 0.3f);
		glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
		float constant = 1.0f;
		float linear = 0.09f;
		float quadratic = 0.032f;

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

	//std::vector<glm::vec3> pointLightPositions = {
	//	glm::vec3(0.7f,  0.2f,  2.0f),
	//	glm::vec3(2.3f, -3.3f, -4.0f),
	//	glm::vec3(-4.0f,  2.0f, -12.0f),
	//	glm::vec3(0.0f,  0.0f, -3.0f)
	//};


}

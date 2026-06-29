#pragma once
#include <glm/glm.hpp>

namespace gl {
	struct Light {
		glm::vec3 position = glm::vec3(1.2f, 1.0f, 2.0f);
		glm::vec3 ambient = glm::vec3(0.2f, 0.2f, 0.2f);
		glm::vec3 diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
		float ambientStrength = 0.1f;
		float specularStrength = 0.5f;
		int shininess = 32;
	};
}

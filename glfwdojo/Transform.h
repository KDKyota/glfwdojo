#pragma once
#include <glm/glm.hpp>

class Transform {
private:
	glm::vec3 position_;
	glm::vec3 rotation_;
	glm::vec3 scale_;
	glm::mat4 modelMatrix_;
	glm::mat3 normalMatrix_;
	bool dirty_ = true;

	void rebuild();

public:
	void setPosition(glm::vec3 p);
	void setRotation(glm::vec3 r);
	void setScale(glm::vec3 s);

	const glm::mat4& getModelMatrix();
	const glm::mat3& getNormalMatrix();
};

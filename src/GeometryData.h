#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <array>

namespace gl {
	struct Vertex {
		glm::vec3 position; // 頂点座標(x, y, z)
		glm::vec3 normal; // 法線ベクトル(x, y, z)
		glm::vec2 uv; // テクスチャ座標(x, y) // lightingの章では使わない
	};

	//inline const std::vector<glm::vec3> cubePositions = {
	//	glm::vec3(0.0f,  0.0f,  0.0f),
	//	glm::vec3(2.0f,  5.0f, -15.0f),
	//	glm::vec3(-1.5f, -2.2f, -2.5f),
	//	glm::vec3(-3.8f, -2.0f, -12.3f),
	//	glm::vec3(2.4f, -0.4f, -3.5f),
	//	glm::vec3(-1.7f,  3.0f, -7.5f),
	//	glm::vec3(1.3f, -2.0f, -2.5f),
	//	glm::vec3(1.5f,  2.0f, -2.5f),
	//	glm::vec3(1.5f,  0.2f, -1.5f),
	//	glm::vec3(-1.3f,  1.0f, -1.5f)
	//};

	inline const std::array<Vertex, 36> rawVertices =
	{ {
		// back face (z = -0.5)
		{{ 0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},

		// front face (z = 0.5)
		{{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 1.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, {}, {0.0f, 1.0f}},
		{{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},

		// left face (x = -0.5)
		{{-0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
		{{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},
		{{-0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},

		// right face (x = 0.5)
		{{ 0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},

		// bottom face (y = -0.5)
		{{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {}, {1.0f, 1.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},

		// top face (y = 0.5)
		{{-0.5f,  0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, {}, {0.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {}, {0.0f, 1.0f}},
	}  };
		// 3頂点の座標(v0, v1, v2)から法線ベクトルを計算する関数
	  inline glm::vec3 calcNormal(const glm::vec3& v0,
		const glm::vec3& v1,
		const glm::vec3& v2)
	  {
		return glm::normalize(glm::cross(v1 - v0, v2 - v1));
	  }

	  // 法線ベクトルを計算するラムダ関数
	  inline const std::array<Vertex, 36> cubeVertices = []() {
		auto v = rawVertices;
		for (int i = 0; i < 36; i += 3) {
		  glm::vec3 n = calcNormal(v[i].position, v[i + 1].position, v[i + 2].position);
		  v[i].normal = v[i + 1].normal = v[i + 2].normal = n;
		}
		return v;
	  }();

}

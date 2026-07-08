#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <array>

#define MAX_BONE_INFLUENCE 4

namespace gl {
	struct Vertex {
		glm::vec3 position; // 頂点座標(x, y, z)
		glm::vec3 normal; // 法線ベクトル(x, y, z)
		glm::vec2 uv; // テクスチャ座標(x, y) 
		glm::vec3 tangent; // 接線ベクトル(x, y, z)
		glm::vec3 bitangent; // 従法線ベクトル(x, y, z)
		int m_BoneIDs[MAX_BONE_INFLUENCE] = { 0 }; // ボーンID
		float m_Weights[MAX_BONE_INFLUENCE] = { 0.0f }; // ボーンの重み
	};

	// 透過色入りのテクスチャ用
	struct TransparentDraw {
		float distance;
		unsigned int index;
	};

	inline const std::vector<glm::vec3> cubePositions = {
		glm::vec3(0.0f,  0.0f,  0.0f),
		glm::vec3(2.0f,  5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f),
		glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3(2.4f, -0.4f, -3.5f),
		glm::vec3(-1.7f,  3.0f, -7.5f),
		glm::vec3(1.3f, -2.0f, -2.5f),
		glm::vec3(1.5f,  2.0f, -2.5f),
		glm::vec3(1.5f,  0.2f, -1.5f),
		glm::vec3(-1.3f,  1.0f, -1.5f)
	};

	inline const std::vector<glm::vec3> windows_pos
    {
        glm::vec3(-1.5f, 0.0f, -0.48f),
        glm::vec3( 1.5f, 0.0f, 0.51f),
        glm::vec3( 0.0f, 0.0f, 0.7f),
        glm::vec3(-0.3f, 0.0f, -2.3f),
        glm::vec3( 0.5f, 0.0f, -0.6f)
    };

	inline const std::vector<glm::vec3> skyboxVertices {
        // positions          
        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),
        glm::vec3(1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),

        glm::vec3(-1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f),

        glm::vec3(1.0f, -1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f,  1.0f),
        glm::vec3(1.0f,  1.0f,  1.0f),
        glm::vec3(1.0f,  1.0f,  1.0f),
        glm::vec3(1.0f,  1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),

        glm::vec3(-1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3(1.0f,  1.0f,  1.0f),
        glm::vec3(1.0f,  1.0f,  1.0f),
        glm::vec3(1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f),

        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3(1.0f,  1.0f, -1.0f),
        glm::vec3(1.0f,  1.0f,  1.0f),
        glm::vec3(1.0f,  1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),

        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f),
        glm::vec3(1.0f, -1.0f,  1.0f)
    };

	inline const float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

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
	} };

	inline const std::array<Vertex, 6> rawplaneVertices =
	{ {
		// positions // normal vectors // texture Coords 
		{{ 5.0f, -0.5f,  5.0f},{}, {2.0f, 0.0f}},
		{{-5.0f, -0.5f,  5.0f},{}, {0.0f, 0.0f}},
		{{-5.0f, -0.5f, -5.0f},{}, {0.0f, 2.0f}},

		{{ 5.0f, -0.5f,  5.0f},{}, {2.0f, 0.0f}},
		{{-5.0f, -0.5f, -5.0f},{}, {0.0f, 2.0f}},
		{{ 5.0f, -0.5f, -5.0f},{}, {2.0f, 2.0f}}
	} };

	inline const std::array<Vertex, 6> rawtransparentVertices =
	{ {
		{{0.0f, 0.5f, 0.0f}, {}, {0.0f, 0.0f}},
		{{0.0f, -0.5f,  0.0f}, {}, {0.0f, 1.0f}},
		{{1.0f, -0.5f,  0.0f}, {}, {1.0f, 1.0f}},

		{{0.0f,  0.5f,  0.0f}, {},  {0.0f,  0.0f}},
		{{1.0f, -0.5f,  0.0f}, {}, {1.0f,  1.0f} },
		{{1.0f,  0.5f,  0.0f}, {}, {1.0f,  0.0f}}
	} };


	// 3頂点の座標(v0, v1, v2)から法線ベクトルを計算する関数
	inline glm::vec3 calcNormal(const glm::vec3& v0,
	const glm::vec3& v1,
	const glm::vec3& v2)
	{
		return glm::normalize(glm::cross(v1 - v0, v2 - v1));
	}
	// 法線ベクトルを計算するラムダ関数
	template <std::size_t N>
	std::array<Vertex, N> calculateNormals(std::array<Vertex, N> vertices)
	{
		for (std::size_t i = 0; i < N; i += 3)
		{
			glm::vec3 n = calcNormal(
				vertices[i].position,
				vertices[i + 1].position,
				vertices[i + 2].position);

			vertices[i].normal =
			vertices[i + 1].normal =
			vertices[i + 2].normal = n;
		}

		return vertices;
	}

	inline const std::array<Vertex, 36> cubeVertices = calculateNormals(rawVertices);
	inline const std::array<Vertex, 6> planeVertices = calculateNormals(rawplaneVertices);
	inline const std::array<Vertex, 6> transparentVertices = calculateNormals(rawtransparentVertices);

}

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

	inline const std::vector<glm::vec3> cube_pos = {
		glm::vec3(-1.0f, 0.0f, -1.0f),
		glm::vec3(2.0f, 0.0f, 0.0f)
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

	// EBO(IBO)用に重複を除いた頂点配列(1面 = 4頂点 x 6面 = 24頂点)
	inline const std::array<Vertex, 24> rawVertices =
	{ {
		// back face (z = -0.5)
		{{ 0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, {}, {0.0f, 1.0f}},

		// front face (z = 0.5)
		{{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, {}, {0.0f, 1.0f}},

		// left face (x = -0.5)
		{{-0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
		{{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},

		// right face (x = 0.5)
		{{ 0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},

		// bottom face (y = -0.5)
		{{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {}, {1.0f, 1.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},

		// top face (y = 0.5)
		{{-0.5f,  0.5f, -0.5f}, {}, {0.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, {}, {0.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
	} };

	// 1面(4頂点)につき2つの三角形を組むためのインデックス配列(0,1,2, 2,3,0 を6面分)
	inline const std::array<unsigned int, 36> cubeIndices =
	{
		 0,  1,  2,  2,  3,  0, // back
		 4,  5,  6,  6,  7,  4, // front
		 8,  9, 10, 10, 11,  8, // left
		12, 13, 14, 14, 15, 12, // right
		16, 17, 18, 18, 19, 16, // bottom
		20, 21, 22, 22, 23, 20, // top
	};

	// EBO用に重複を除いた頂点配列(4頂点の四角形)
	inline const std::array<Vertex, 4> rawplaneVertices =
	{ {
		// positions // normal vectors // texture Coords
		{{ 5.0f, -0.5f,  5.0f},{}, {2.0f, 0.0f}},
		{{-5.0f, -0.5f,  5.0f},{}, {0.0f, 0.0f}},
		{{-5.0f, -0.5f, -5.0f},{}, {0.0f, 2.0f}},
		{{ 5.0f, -0.5f, -5.0f},{}, {2.0f, 2.0f}},
	} };

	inline const std::array<unsigned int, 6> planeIndices = { 0, 1, 2, 2, 3, 0 };

	// EBO用に重複を除いた頂点配列(4頂点の四角形)
	inline const std::array<Vertex, 4> rawtransparentVertices =
	{ {
		{{0.0f,  0.5f, 0.0f}, {}, {0.0f, 0.0f}},
		{{0.0f, -0.5f, 0.0f}, {}, {0.0f, 1.0f}},
		{{1.0f, -0.5f, 0.0f}, {}, {1.0f, 1.0f}},
		{{1.0f,  0.5f, 0.0f}, {}, {1.0f, 0.0f}},
	} };

	inline const std::array<unsigned int, 6> transparentIndices = { 0, 1, 2, 2, 3, 0 };


	// 3頂点の座標(v0, v1, v2)から法線ベクトルを計算する関数
	inline glm::vec3 calcNormal(const glm::vec3& v0,
	const glm::vec3& v1,
	const glm::vec3& v2)
	{
		return glm::normalize(glm::cross(v1 - v0, v2 - v1));
	}

	// 頂点配列が「1面 = 4頂点」の並びである前提で、面ごとに法線を計算して4頂点に割り当てる関数
	// (EBOで頂点を共有するため、三角形単位ではなく面単位でまとめて処理する)
	template <std::size_t N> // Nは頂点数（インデックス数）
	std::array<Vertex, N> calculateFaceNormals(std::array<Vertex, N> vertices)
	{
		// Nが4の倍数であることをコンパイル時にチェック(falseならエラー)
		static_assert(N % 4 == 0, "calculateFaceNormals expects 4 vertices per face");

		for (std::size_t i = 0; i < N; i += 4)
		{
			glm::vec3 n = calcNormal(
				vertices[i].position,
				vertices[i + 1].position,
				vertices[i + 2].position);

			vertices[i].normal =
			vertices[i + 1].normal =
			vertices[i + 2].normal =
			vertices[i + 3].normal = n;
		}

		return vertices;
	}

	inline const std::array<Vertex, 24> cubeVertices = calculateFaceNormals(rawVertices);
	inline const std::array<Vertex, 4> planeVertices = calculateFaceNormals(rawplaneVertices);
	inline const std::array<Vertex, 4> transparentVertices = calculateFaceNormals(rawtransparentVertices);

}

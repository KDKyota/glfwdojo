#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "Lighting.h"

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
		glm::vec3(-1.0f, 1.0f, -1.0f),
		glm::vec3(2.0f,  1.5f,  0.0f),
		glm::vec3(0.0f,  1.0f, -20.0f), // z=-25 壁の前
		glm::vec3(0.0f,  1.0f,  20.0f), // z=+25 壁の前
	};

	inline const std::array<gl::PointLight, 1> pointLights = {{
		//{ glm::vec3( 0.7f,  0.2f,  2.0f) },
		//{ glm::vec3( 2.3f, -3.3f, -4.0f) },
		//{ glm::vec3(-4.0f,  2.0f,-12.0f) },
		//{ glm::vec3( 0.0f,  0.0f, -3.0f) },
		{glm::vec3(-5.0f, 7.0f, 0.0f)}
	}};

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
	// cubeのローカル座標
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

	// EBO用に重複を除いた頂点配列(床)
	inline const std::array<Vertex, 4> rawplaneVertices =
	{ {
		// positions // normal vectors // texture Coords
		{{-25.0f, -0.5f,  25.0f},{}, {0.0f,  0.0f}},
		{{ 25.0f, -0.5f,  25.0f},{}, {25.0f, 0.0f}},
		{{ 25.0f, -0.5f, -25.0f},{}, {25.0f, 25.0f}},
		{{-25.0f, -0.5f, -25.0f},{}, {0.0f,  25.0f}},
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

	inline std::array<glm::vec3, 2> calcTangentBitangent(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2
		, const glm::vec2& uv0, const glm::vec2 uv1, const glm::vec2 uv2)
	{
		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		glm::vec3 tangent, bitangent;
		tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		tangent = glm::normalize(tangent);

		bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
		bitangent = glm::normalize(bitangent);

		return std::array<glm::vec3, 2> {tangent, bitangent};
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

	template <std::size_t N>
	// TangentとBitangentを計算する関数
	std::array<Vertex, N> calculateTangentBitangent(std::array<Vertex, N> vertices)
	{
		static_assert(N % 4 == 0, "calculateTangentBitangent expects 4 vertices per face");

		for (std::size_t i = 0; i < N; i += 4)
		{
			auto [tangent, bitangent] = calcTangentBitangent(
				vertices[i].position, vertices[i + 1].position, vertices[i + 2].position,
				vertices[i].uv, vertices[i + 1].uv, vertices[i + 2].uv);

			vertices[i].tangent =
			vertices[i + 1].tangent =
			vertices[i + 2].tangent =
			vertices[i + 3].tangent = tangent;

			vertices[i].bitangent =
			vertices[i + 1].bitangent =
			vertices[i + 2].bitangent =
			vertices[i + 3].bitangent = bitangent;
		}

		return vertices;
	}

	inline const std::array<Vertex, 24> cubeVertices = calculateTangentBitangent(calculateFaceNormals(rawVertices));
	inline const std::array<Vertex, 4> planeVertices = calculateFaceNormals(rawplaneVertices);
	inline const std::array<Vertex, 4> transparentVertices = calculateFaceNormals(rawtransparentVertices);

	// 壁の頂点データ（z=-25 と z=+25 の2枚）
	// Normal, Tangent, Bitangent は解析的に設定（T×B = N が成立）
	inline const std::array<Vertex, 8> wallVertices = { {
		// z=-25 の壁 (法線: +z,  T: +x, B: +y)
		{{ -25.0f, -0.5f, -25.0f }, { 0.0f, 0.0f,  1.0f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		{{  25.0f, -0.5f, -25.0f }, { 0.0f, 0.0f,  1.0f }, { 5.0f, 0.0f }, {  1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		{{  25.0f, 10.0f, -25.0f }, { 0.0f, 0.0f,  1.0f }, { 5.0f, 1.0f }, {  1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		{{ -25.0f, 10.0f, -25.0f }, { 0.0f, 0.0f,  1.0f }, { 0.0f, 1.0f }, {  1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		// z=+25 の壁 (法線: -z,  T: -x, B: +y)
		{{  25.0f, -0.5f,  25.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		{{ -25.0f, -0.5f,  25.0f }, { 0.0f, 0.0f, -1.0f }, { 5.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		{{ -25.0f, 10.0f,  25.0f }, { 0.0f, 0.0f, -1.0f }, { 5.0f, 1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		{{  25.0f, 10.0f,  25.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
	} };

	inline const std::array<unsigned int, 12> wallIndices = {
		0, 1, 2,  2, 3, 0,   // z=-25 壁
		4, 5, 6,  6, 7, 4,   // z=+25 壁
	};

}

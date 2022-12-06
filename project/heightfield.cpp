
#include "heightfield.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>

using namespace glm;
using std::string;

HeightField::HeightField(void)
    : m_meshResolution(0)
    , m_heightFieldPath("")
    , m_diffuseTexturePath("")
    , m_texid_hf(UINT32_MAX)
    , m_texid_diffuse(UINT32_MAX)
    , m_vao(UINT32_MAX)
    , m_positionBuffer(UINT32_MAX)
    , m_uvBuffer(UINT32_MAX)
    , m_indexBuffer(UINT32_MAX)
    , m_numIndices(0)
{
}

void HeightField::loadHeightField(const std::string& heigtFieldPath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	float* data = stbi_loadf(heigtFieldPath.c_str(), &width, &height, &components, 1);
	if(data == nullptr)
	{
		std::cout << "Failed to load image: " << heigtFieldPath << ".\n";
		return;
	}

	if(m_texid_hf == UINT32_MAX)
	{
		glGenTextures(1, &m_texid_hf);
	}
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT,
	             data); // just one component (float)

	m_heightFieldPath = heigtFieldPath;
	std::cout << "Successfully loaded heigh field texture: " << heigtFieldPath << ".\n";
}

void HeightField::loadDiffuseTexture(const std::string& diffusePath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	uint8_t* data = stbi_load(diffusePath.c_str(), &width, &height, &components, 3);
	if(data == nullptr)
	{
		std::cout << "Failed to load image: " << diffusePath << ".\n";
		return;
	}

	if(m_texid_diffuse == UINT32_MAX)
	{
		glGenTextures(1, &m_texid_diffuse);
	}

	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // plain RGB
	glGenerateMipmap(GL_TEXTURE_2D);

	std::cout << "Successfully loaded diffuse texture: " << diffusePath << ".\n";
}


void HeightField::generateMesh(int tesselation)
{
	// Generate a mesh in range -1 to 1 in x and z
	// (y is 0 but will be altered in height field vertex shader)
	// Here, the tesselation is the number of triangles per side.
	m_meshResolution = tesselation;

	// We tesselate by starting in the lower left corner and assigning
	// indices like so:
	//
	//       N edges
	// ┌─────────────────┐
	// ┌─────┬─────┬─────┐y+1   v+N+1┌─────┐v+N+2
	// │╲    │     │╲    │           │╲  B │
	// │ ╲   │     │ ╲   │           │ ╲   │
	// │  ╲  │ ... │  ╲  │           │  ╲  │
	// │   ╲ │     │   ╲ │           │   ╲ │
	// │    ╲│     │    ╲│           │ A  ╲│
	// └───────────┴─────┘y         v└─────┘v+1
	// ▲                 ▲
	// └─y(N + 1)        └─y(N + 1) + N
	//
	// Since we use counter-clockwise winding order, we define two triangles:
	// - A connects v, v+1, v+N+1
	// - B connects v+1, v+N+2, v+N+1

	const int n = tesselation;
	const int vertexCount = (n + 1) * (n + 1);
	std::vector<vec2> posData, uvData;
	std::vector<vec3> indexData;

	posData.reserve(vertexCount);
	uvData.reserve(vertexCount);
	indexData.reserve(tesselation * n * 2 * 3);

	int v = 0;
	for(int y = 0; y <= n; ++y)
	{
		const float y_pos = y / (float)n;
		for(int x = 0; x <= n; ++x)
		{
			const float x_pos = x / (float)n;
			// Set position and UV coordinate data.
			posData.emplace_back(-1.0 + 2.0 * x_pos, -1.0 + 2.0 * y_pos);
			uvData.emplace_back(x_pos, y_pos);
			// Set triangle A and B indices.
			indexData.emplace_back(v, v + 1, v + n + 1);
			indexData.emplace_back(v + 1, v + n + 2, v + n + 1);
			++v;
		}
	}

	// Push everything to the GPU.
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	CHECK_GL_ERROR();

	if(m_indexBuffer == UINT32_MAX)
		glGenBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size() * sizeof(vec2), indexData.data(), GL_STATIC_DRAW);

	if(m_positionBuffer == UINT32_MAX)
		glGenBuffers(1, &m_positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, posData.size() * sizeof(vec2), posData.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(0);

	if(m_uvBuffer == UINT32_MAX)
		glGenBuffers(1, &m_uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, uvData.size() * sizeof(vec3), uvData.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(1);

	CHECK_GL_ERROR();
}

void HeightField::submitTriangles(void)
{
	if(m_vao == UINT32_MAX)
	{
		std::cout << "No vertex array is generated, cannot draw anything.\n";
		return;
	}
}
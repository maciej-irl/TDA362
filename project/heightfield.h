#include <string>
#include <GL/glew.h>

class HeightField
{
public:
	// Triangles edges per quad side
	int m_meshResolution;
	// Textures.
	std::string m_heightFieldPath;
	std::string m_diffuseTexturePath;
	GLuint m_texid_hf;
	GLuint m_texid_diffuse;
	// Our VAO and its buffers.
	GLuint m_vao;
	GLuint m_positionBuffer;
	GLuint m_uvBuffer;
	GLuint m_indexBuffer;
	GLuint m_numIndices;

	HeightField(void);

	/// Load height field
	void loadHeightField(const std::string& heigtFieldPath);

	/// Load diffuse map
	void loadDiffuseTexture(const std::string& diffusePath);

	/// Generate mesh
	void generateMesh(int tesselation);

	/// Render height map
	void submitTriangles(void);
};

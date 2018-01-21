#pragma once 

struct RenderDevice;
struct Mesh;

class ThunderBallApp : public Application
{
public:
	ThunderBallApp();
	~ThunderBallApp();

	void initialize();
	void update();
	void render();
	void resize(uint32_t width, uint32_t height);

protected:
	Mesh *m_meshes;

	uint32_t m_numMeshes;
};

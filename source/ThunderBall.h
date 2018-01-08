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

protected:
	RenderDevice *m_pRenderDevice;
	Mesh *m_meshes;

	uint32_t m_numMeshes;
};

#pragma once 

struct RenderDevice;
struct Mesh;

class ThunderBallApp : public Application
{
public:
	ThunderBallApp();
	~ThunderBallApp();

	void initialize(ScopeStack& scopeStack, RenderDevice& renderDevice);
	void update(ScopeStack& frameScope, RenderDevice& renderDevice);
	void render(ScopeStack& frameScope, RenderDevice& renderDevice);

protected:
	Mesh **m_meshes;
	uint32_t m_numMeshes;
};

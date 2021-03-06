#pragma once 

struct RenderDevice;
struct Mesh;
struct Scene;

class VulkanTest : public Application
{
public:
	VulkanTest();
	~VulkanTest();

	void initialize(ScopeStack& scopeStack, RenderDevice& renderDevice);
	void update(ScopeStack& frameScope, RenderDevice& renderDevice);
	void render(ScopeStack& frameScope, RenderDevice& renderDevice);

protected:
	Mesh **m_meshes;
	uint32_t m_numMeshes;
	Scene *m_scene;
	glm::vec3 *m_rotationAxis;
	glm::vec3 *m_positions;
};

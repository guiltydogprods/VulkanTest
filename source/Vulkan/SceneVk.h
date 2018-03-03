#pragma once

struct Mesh;
struct Buffer;

struct Scene
{
	Scene(ScopeStack& scope, RenderDevice& renderDevice, uint32_t maxMeshes);
	~Scene();

	uint32_t addMeshInstance(Mesh *mesh, glm::mat4x4& worldMatrix);

	void update(ScopeStack& scope);
	void render(ScopeStack& scope);

	RenderDevice& m_renderDevice;
	Mesh		**m_instanceMeshes;
	glm::mat4x4 *m_instanceMatrices;

	Buffer		*m_modelMatrixUniformBuffer;

	uint32_t m_capacity;
	uint32_t m_meshInstanceCount;
	uint32_t m_renderableCount;
};

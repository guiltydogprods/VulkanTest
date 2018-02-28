#pragma once

struct Mesh;

struct Scene
{
	Scene(ScopeStack& scope, RenderDevice& renderDevice, uint32_t maxMeshes);
	~Scene();

	uint32_t addMeshInstance(Mesh *mesh, glm::mat4x4& worldMatrix);

	RenderDevice& m_renderDevice;
	Mesh		**m_instanceMeshes;
	glm::mat4x4 *m_instanceMatrices;

	uint32_t m_capacity;
	uint32_t m_meshInstanceCount;
	uint32_t m_renderableCount;
};

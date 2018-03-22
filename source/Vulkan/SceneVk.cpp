#include "stdafx.h"
#include "SceneVk.h"
#include "RenderDeviceVk.h"
#include "BufferVk.h"

#include "../Framework/Mesh.h"

Scene::Scene(ScopeStack& scope, RenderDevice& renderDevice, uint32_t capacity)
	: m_renderDevice(renderDevice)
	, m_instanceMeshes(nullptr)
	, m_instanceMatrices(nullptr)
	, m_modelMatrixUniformBuffer(nullptr)
	, m_capacity(0)
	, m_meshInstanceCount(0)
	, m_renderableCount(0)
{
	m_instanceMeshes = static_cast<Mesh **>(scope.allocate(sizeof(Mesh *) * capacity));
	m_instanceMatrices = static_cast<glm::mat4x4 *>(scope.allocate(sizeof(glm::mat4x4) * capacity));

	m_sceneUniformBuffer = scope.newObject<Buffer>(scope, renderDevice, sizeof(SceneUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	m_sceneUniformBuffer->bindMemory();

	m_modelMatrixUniformBuffer = scope.newObject<Buffer>(scope, renderDevice, sizeof(glm::mat4x4) * capacity, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	m_modelMatrixUniformBuffer->bindMemory();
}

Scene::~Scene()
{

}

uint32_t Scene::addMeshInstance(Mesh *mesh, glm::mat4x4& worldMatrix)
{
	uint32_t instanceIndex = m_renderableCount;

	m_instanceMeshes[m_meshInstanceCount] = mesh;
	m_instanceMatrices[m_meshInstanceCount] = worldMatrix;

	m_renderableCount += mesh->m_numRenderables;
	m_meshInstanceCount++;

	return instanceIndex;
}

void Scene::update(ScopeStack& scope)
{
}

void Scene::render(ScopeStack& scope)
{
	m_renderDevice.render(scope);
}

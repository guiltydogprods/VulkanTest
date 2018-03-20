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
	glm::vec3 eye(0.0f, 0.0f, 2.5f);
	glm::vec3 at(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::mat4x4 viewMatrix = glm::lookAt(eye, at, up);

	Application* app = Application::GetApplication();

	const float fov = glm::radians(90.0f);
	const float aspectRatio = (float)app->getScreenHeight() / (float)app->getScreenWidth();
	const float nearZ = 0.1f;
	const float farZ = 100.0f;
	const float focalLength = 1.0f / tanf(fov * 0.5f);

	float left = -nearZ / focalLength;
	float right = nearZ / focalLength;
	float bottom = -aspectRatio * nearZ / focalLength;
	float top = aspectRatio * nearZ / focalLength;

	glm::mat4x4 projectionMatrix = glm::frustum(left, right, bottom, top, nearZ, farZ);

	SceneUniformData *sceneUniformData = static_cast<SceneUniformData *>(m_sceneUniformBuffer->mapMemory());
	sceneUniformData->viewMatrix = viewMatrix;
	sceneUniformData->projectionMatrix = projectionMatrix;
	sceneUniformData->viewProjectionMatrix = projectionMatrix * viewMatrix;
	m_sceneUniformBuffer->unmapMemory();
}

void Scene::render(ScopeStack& scope)
{
	m_renderDevice.render(scope);
}

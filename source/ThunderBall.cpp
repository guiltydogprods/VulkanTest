#include "stdafx.h"
#include "ThunderBall.h"
#include "Framework/Mesh.h"
#include "Framework/ResourceManager.h"
#include "Vulkan/BufferVk.h"
#include "Vulkan/RenderDeviceVk.h"
#include "Vulkan/RenderTargetVk.h"
#include "Vulkan/SceneVk.h"

ThunderBallApp s_thundeBallApp;

const char		*kApplicationName = "Thunder Ball";

ThunderBallApp::ThunderBallApp()
	: Application(kApplicationName)
	, m_meshes(nullptr)
	, m_numMeshes(0)
	, m_scene(nullptr)
{
}

ThunderBallApp::~ThunderBallApp()
{
	print("ThunderBallApp::dtor\n");
}

void ThunderBallApp::initialize(ScopeStack& scope, RenderDevice& renderDevice)
{
	uint32_t verticesSize = 100000;
	uint32_t indicesSize = 100000;

	renderDevice.createVertexFormat(scope);
	renderDevice.m_vertexBuffer = scope.newObject<Buffer>(scope, renderDevice, verticesSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	renderDevice.m_vertexBuffer->bindMemory();
	renderDevice.m_indexBuffer = scope.newObject<Buffer>(scope, renderDevice, indicesSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	renderDevice.m_indexBuffer->bindMemory();

	ResourceName resources[] =
	{
		{ "Sphere.s3d",	kRTMesh		},
		{ "Donut2.s3d",	kRTMesh		},
		{ "box.s3d", kRTMesh		},
		{ "stone34.dds", kRTTexture },
		{ "rock7.dds", kRTTexture },
	};

	ResourceManager *resourceManager = scope.newObject<ResourceManager>(scope, renderDevice, resources);

	uint32_t numMeshes = resourceManager->m_numMeshes;
	Mesh **meshes = resourceManager->m_meshes;
	uint32_t numTextures = resourceManager->m_numTextures;
	Texture **textures = resourceManager->m_textures;

	renderDevice.createUniformBuffers(scope);

	const uint32_t kNumMeshInstances = 2;
	m_scene = scope.newObject<Scene>(scope, renderDevice, kNumMeshInstances);

	renderDevice.finalize(scope, *m_scene, textures,  numTextures);

	glm::vec3 axis[kNumMeshInstances] =
	{
		{ 0.707f, 0.0f, 0.707f },
		{ -0.707f, 0.0f, -0.707f },
	};

	glm::vec3 positions[kNumMeshInstances] = 
	{
		{ -0.5f, 0.0f, 0.0f },
		{  0.5f, 0.0f, 0.0f },
	};

	m_rotationAxis = static_cast<glm::vec3 *>(scope.allocate(sizeof(glm::vec3) * numMeshes));
	m_positions = static_cast<glm::vec3 *>(scope.allocate(sizeof(glm::vec3) * numMeshes));
	for (uint32_t i = 0; i < kNumMeshInstances; ++i)
	{
		m_rotationAxis[i] = axis[i];
		m_positions[i] = positions[i];
	}

	float angle = 0.0f;
	glm::mat4x4 modelMatrix0 = glm::translate(m_positions[0]) * glm::rotate(glm::radians(angle), m_rotationAxis[0]);
	glm::mat4x4 modelMatrix1 = glm::translate(m_positions[1]) * glm::rotate(glm::radians(angle), m_rotationAxis[1]);

	uint32_t meshIndex0 = m_scene->addMeshInstance(meshes[0], modelMatrix0);
	uint32_t meshIndex1 = m_scene->addMeshInstance(meshes[1], modelMatrix1);
}

void ThunderBallApp::update(ScopeStack& scope, RenderDevice& renderDevice)
{
	static float angle = 0.0f;
	uint32_t numMeshes = m_scene->m_meshInstanceCount;

	glm::mat4x4 *instanceMatrices = static_cast<glm::mat4x4 *>(m_scene->m_modelMatrixUniformBuffer->mapMemory());

	for (uint32_t i = 0; i < numMeshes; ++i)
	{
		glm::mat4x4& instanceMatrix = instanceMatrices[i];
		instanceMatrix = glm::translate(m_positions[i]) * glm::rotate(glm::radians(angle), m_rotationAxis[i]);
	}
	m_scene->m_modelMatrixUniformBuffer->unmapMemory();

	angle += 1.0f;
	if (angle > 360.0f)
		angle -= 360.0f;

	m_scene->update(scope);
}

void ThunderBallApp::render(ScopeStack& frameScope, RenderDevice& renderDevice)
{
	m_scene->render(frameScope);
}

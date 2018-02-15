#include "stdafx.h"
#include "ThunderBall.h"
#include "Framework/Mesh.h"
#include "Framework/ResourceManager.h"
#include "Vulkan/BufferVk.h"
#include "Vulkan/RenderDeviceVk.h"

ThunderBallApp s_thundeBallApp;

const char		*kApplicationName = "Thunder Ball";

ThunderBallApp::ThunderBallApp()
	: Application(kApplicationName)
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
	renderDevice.finalize(scope, meshes, numMeshes, textures,  numTextures);
}

void ThunderBallApp::update(ScopeStack& frameScope, RenderDevice& renderDevice)
{
	renderDevice.update();
}

void ThunderBallApp::render(ScopeStack& frameScope, RenderDevice& renderDevice)
{
	renderDevice.render(frameScope);
}

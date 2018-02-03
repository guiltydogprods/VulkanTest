#include "stdafx.h"
#include "ThunderBall.h"
#include "Mesh.h"
#include "Vulkan/RenderDeviceVk.h"

ThunderBallApp s_thundeBallApp;

const char		*kApplicationName = "Thunder Ball";
const uint32_t	kScreenWidth = 1920;
const uint32_t	kScreenHeight = 1080;

ThunderBallApp::ThunderBallApp()
	: Application(kApplicationName, kScreenWidth, kScreenHeight)
{
}

ThunderBallApp::~ThunderBallApp()
{
	print("ThunderBallApp::dtor\n");
}

void ThunderBallApp::initialize(ScopeStack& scopeStack)
{
	m_scopeStack = &scopeStack;

	m_pRenderDevice = scopeStack.newObject<RenderDevice>(scopeStack);

	const char *meshes[] =
	{
		"Donut2.s3d",
		"Sphere.s3d",
//		"box.s3d"
	};

	uint32_t verticesSize = 100000;
	uint32_t indicesSize = 100000;

	m_pRenderDevice->createVertexFormat();
	m_pRenderDevice->m_vertexBuffer = scopeStack.newObject<Buffer>(scopeStack, *m_pRenderDevice, verticesSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_pRenderDevice->m_vertexBuffer->bindMemory();
	int64_t vertexBufferOffset = 0;
	m_pRenderDevice->m_indexBuffer = scopeStack.newObject<Buffer>(scopeStack, *m_pRenderDevice, indicesSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_pRenderDevice->m_indexBuffer->bindMemory();
	int64_t indexBufferOffset = 0;

	m_numMeshes = sizeof(meshes) / sizeof(const char *);
	m_meshes = static_cast<Mesh **>(scopeStack.allocate(sizeof(Mesh *) * m_numMeshes));

	for (uint32_t i = 0; i < m_numMeshes; ++i)
	{
		m_meshes[i] = scopeStack.newObject<Mesh>(scopeStack, meshes[i], *m_pRenderDevice, vertexBufferOffset, indexBufferOffset);
	}

	m_pRenderDevice->createUniformBuffer(scopeStack);
	m_pRenderDevice->finalize(m_meshes, m_numMeshes);
}

void ThunderBallApp::update(ScopeStack& frameScope)
{
	m_pRenderDevice->update();
}

void ThunderBallApp::render(ScopeStack& frameScope)
{
	m_pRenderDevice->render();
}

void ThunderBallApp::resize(uint32_t width, uint32_t height)
{
	m_pRenderDevice->recreateSwapChain();
	m_screenWidth = width;
	m_screenHeight = height;
}

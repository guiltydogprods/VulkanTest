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
//	, m_pRenderDevice(nullptr)
{
}

ThunderBallApp::~ThunderBallApp()
{
	Mesh* pMesh = m_meshes + m_numMeshes;
	while (pMesh-- > m_meshes)
		pMesh->~Mesh();

	free(m_meshes);
	m_meshes = 0;

	delete m_pRenderDevice;
	m_pRenderDevice = 0;
}

void ThunderBallApp::initialize()
{
	m_pRenderDevice = new RenderDevice();

	const char *meshes[] =
	{
		"Donut2.s3d",
//		"Sphere.s3d",
//		"box.s3d"
	};

	uint32_t verticesSize = 100000;
	uint32_t indicesSize = 100000;
	m_pRenderDevice->m_vertexBuffer = new Buffer(*m_pRenderDevice, verticesSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_pRenderDevice->m_vertexBuffer->bindMemory();
	int64_t vertexBufferOffset = 0;
	m_pRenderDevice->m_indexBuffer = new Buffer(*m_pRenderDevice, indicesSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_pRenderDevice->m_indexBuffer->bindMemory();
	int64_t indexBufferOffset = 0;

	m_numMeshes = sizeof(meshes) / sizeof(const char *);
	Mesh* meshAddr = m_meshes = static_cast<Mesh *>(malloc(sizeof(Mesh) * m_numMeshes));

	for (uint32_t i = 0; i < m_numMeshes; ++i, ++meshAddr)
		new (meshAddr) Mesh(meshes[i], *m_pRenderDevice->m_vertexBuffer, vertexBufferOffset, *m_pRenderDevice->m_indexBuffer, indexBufferOffset);

	m_pRenderDevice->finalize(m_meshes, m_numMeshes);
}


void ThunderBallApp::update()
{
	m_pRenderDevice->update();
}

void ThunderBallApp::render()
{
	m_pRenderDevice->render();
}

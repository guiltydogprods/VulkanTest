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
	, m_pRenderDevice(nullptr)
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

	Buffer& vertexBuffer = *m_pRenderDevice->m_vertexBuffer;
	int64_t vertexBufferOffset = 0;
	Buffer& indexBuffer = *m_pRenderDevice->m_indexBuffer;
	int64_t indexBufferOffset = 0;

	m_numMeshes = sizeof(meshes) / sizeof(const char *);
	Mesh* meshAddr = m_meshes = static_cast<Mesh *>(malloc(sizeof(Mesh) * m_numMeshes));

	for (uint32_t i = 0; i < m_numMeshes; ++i, ++meshAddr)
		new (meshAddr) Mesh(meshes[i], vertexBuffer, vertexBufferOffset, indexBuffer, indexBufferOffset);

	m_pRenderDevice->finalize();
}


void ThunderBallApp::update()
{
	m_pRenderDevice->update();
}

void ThunderBallApp::render()
{
	m_pRenderDevice->render();
}

#pragma once

#include <atomic>
#include <functional>

struct Mesh;
struct Texture;

enum ResourceType : uint32_t
{
	kRTNone = 0,
	kRTMesh,
	kRTTexture,
};

struct ResourceName
{
	const char		*resourceFilename;
	ResourceType	resourceType;
};

struct ResourceInfo
{
	ResourceInfo()
		: m_type(kRTNone)
		, m_filenameHash(0)
		, m_sizeInBytes(0)
	{}
	~ResourceInfo();

	ResourceType	m_type;
	uint32_t		m_filenameHash;
	uint64_t		m_sizeInBytes;
};

struct MeshResourceInfo : public ResourceInfo
{
	uint32_t	m_numVertices;
	uint32_t	m_numIndices;
	uint32_t	m_numNodes;
	uint32_t	m_numTextures;
	uint32_t	m_numMaterials;						//CLR I think, the number of Materials and Renderables "should" always be the same?
	uint32_t	m_numRenderables;
};

struct TextureResourceInfo : public ResourceInfo
{
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_linearSize;
	uint32_t m_mipMapCount;
	uint32_t m_fourCC;
	bool	 m_bSrgb;
};

struct ResourceManager
{
	template<size_t N>
	ResourceManager(ScopeStack& scope, RenderDevice& renderDevice, ResourceName(&resources)[N])
		: m_meshes(nullptr)
		, m_textures(nullptr)
		, m_numMeshes(0)
		, m_numTextures(0)
	{
		registerResources(scope, renderDevice, N, &resources[0]);
	}

	~ResourceManager() {}

	void registerResources(ScopeStack&scope, RenderDevice& renderDevice, uint32_t numResources, ResourceName *resources);

	Mesh				**m_meshes;
	Texture				**m_textures;
	uint32_t			m_numMeshes;
	uint32_t			m_numTextures;
};

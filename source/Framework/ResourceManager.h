#pragma once

#include <atomic>
#include <functional>

struct Mesh;
struct Texture;

enum ResourceType : uint16_t
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

struct ResourceFilename
{
	ResourceFilename(ScopeStack& scope, const char *resourceFilename)
		: filenameLength(static_cast<uint16_t>(strlen(resourceFilename)))
	{
		size_t size = sizeof(filename);
		if (filenameLength > sizeof(filename)-1)
		{
			AssertMsg(false, "More space needs to be allocated.\n");
		}
		memcpy(filename, resourceFilename, filenameLength);
		filename[filenameLength] = 0;
	}

	ResourceFilename()
		: filenameLength(0) {}

	uint16_t filenameLength;				//2 bytes
	char filename[60];						//60 bytes - if filename is longer than 59 + 1 characters, allocate the remaining bytes immediately afterwards.
};											//62 bytes total

struct ResourceEntry
{
	ResourceEntry(ScopeStack& scope, const char *_resourceFilename, ResourceType _type)
		: resourceType(_type)
		, resourceFilename(scope, _resourceFilename) {}

	ResourceEntry()
		: resourceType(kRTNone) {}

	ResourceType		resourceType;		//2 bytes
	ResourceFilename	resourceFilename;	//62 bytes
};											//64 bytes total

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
		: m_resourceList(nullptr)
		, m_meshes(nullptr)
		, m_textures(nullptr)
		, m_resourceCount(0)
		, m_numMeshes(0)
		, m_numTextures(0)
	{
		for (size_t i = 0; i < N; ++i)
		{
			registerResource(scope, resources[i]);
		}
		loadResources(scope, renderDevice);
	}

	~ResourceManager() {}

	void registerResource(ScopeStack& scope, ResourceName& resource);
	void registerResources(ScopeStack& scope, RenderDevice& renderDevice, uint32_t numResources, ResourceName *resources);
	void loadResources(ScopeStack&scope, RenderDevice& renderDevice);

	ResourceEntry		*m_resourceList;
	Mesh				**m_meshes;
	Texture				**m_textures;
	uint32_t			m_resourceCount;
	uint32_t			m_numMeshes;
	uint32_t			m_numTextures;
};

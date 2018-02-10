#pragma once

#include "glm/glm.hpp"

const int kVersionMajor = 0;
const int kVersionMinor = 1;

struct ChunkId
{
	char	tag[4];
	uint32_t size;
};

struct ModelHeader
{
	ChunkId		chunkId;
	uint32_t	versionMajor;
	uint32_t	versionMinor;
};

struct RenderableInfo
{
	uint32_t	numVertices;
	uint32_t	numIndices;
	uint32_t	materialHash;
	float		aabbMinX;
	float		aabbMinY;
	float		aabbMinZ;
	float		aabbMaxX;
	float		aabbMaxY;
	float		aabbMaxZ;
	uint32_t	verticesOffset;
	uint32_t	indicesOffset;
};

struct MeshInfo
{
	glm::mat4x4	worldMatrix;
	float		aabbMinX;
	float		aabbMinY;
	float		aabbMinZ;
	float		aabbMaxX;
	float		aabbMaxY;
	float		aabbMaxZ;
	uint32_t	numRenderables;
	uint32_t	numChildren;
};

struct MeshChunk
{
	ChunkId		chunkId;
	uint32_t	numMeshes;
};

struct TextureInfo
{
	uint32_t width;
	uint32_t height;
	uint32_t textureOffset;
};

struct TextureChunk
{
	ChunkId chunkId;
	uint32_t numTextures;
};

struct MaterialInfo
{
	float		diffuse[3];
	float		ambient[3];
	float		specular[4];
	float		alpha;
	uint32_t	flags;
	float		reflectVal;
	uint32_t	diffuseMapHash;
	uint32_t	normalMapHash;
};

struct MaterialChunk
{
	ChunkId chunkId;
	uint32_t numMaterials;
};

struct MeshNode
{
	int32_t	 m_parentId;
	uint32_t m_numRenderables;
	uint32_t m_numChildren;
};

struct Renderable
{
	Renderable(uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount, uint32_t materialIndex);

	uint32_t getFirstVertex() { return m_firstVertex; }
	uint32_t getFirstIndex() { return m_firstIndex; }
	uint32_t getIndexCount() { return m_indexCount; }
	int32_t getMaterialIndex() { return m_materialIndex; }

	uint32_t	m_firstVertex;
	uint32_t	m_firstIndex;
	uint32_t	m_indexCount;
	int32_t		m_materialIndex;
};

struct Buffer;

struct Mesh
{
	Mesh(ScopeStack& scopeStack, const char *filename, RenderDevice& renderDevice, int64_t& vertexBufferOffset, int64_t& indexBufferOffset);
	~Mesh();

	void		processMeshChunk(ScopeStack& scopeStack, File& file, RenderDevice& renderDevice, int64_t& vertexBufferOffset, int64_t& indexBufferOffset);
	uint8_t*	processMeshRecursive(uint8_t *ptr, uint32_t& renderableIndex, uint32_t& nodeIndex, int32_t parentIndex, RenderDevice& renderDevice, int64_t& vertexBufferOffset, int64_t& indexBufferOffset);


	MeshNode	*m_hierarchy;
	glm::mat4x4	*m_transforms;
	Renderable	*m_renderables;
//	Material	*m_materials;
	uint32_t	m_numNodes;
	uint32_t	m_numRenderables;
	uint32_t	m_numTextures;
	uint32_t	m_numMaterials;
	float		m_aabbMin[3];
	float		m_aabbMax[3];
};

namespace Import
{
	static const uint32_t kMaxVertexElems = 16;
	static const uint32_t kMaxStreamCount = 2;

	struct VertexElement
	{
		uint8_t			m_index;
		int8_t			m_size;
		uint16_t		m_type;
		uint8_t			m_normalized;
		uint8_t			m_offset;			//CLR - is this big enough for MultiDrawIndirect?
	};

	struct VertexStream
	{
		VertexStream();
		void addElement(uint8_t index, int8_t size, uint16_t type, uint8_t normalized, uint8_t offset);

		uint32_t				m_glBufferId;
		uint8_t					m_bufferType;
		uint8_t					m_numElements;
		uint16_t				m_stride;
		uint32_t				m_dataOffset;
		Import::VertexElement	m_elements[kMaxVertexElems];
	};

	struct VertexBuffer
	{
		VertexBuffer();

		inline VertexStream*	getVertexStreams() { return m_streams; }

		uint32_t		m_numStreams;
		VertexStream	m_streams[kMaxStreamCount];
	};
}


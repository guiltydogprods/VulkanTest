#include "stdafx.h"
#include "Mesh.h"
#include "Framework/File.h"

Mesh::Mesh(const char *filename)
	: m_hierarchy(nullptr)
	, m_transforms(nullptr)
	, m_renderables(nullptr)
//	, m_materials(nullptr)
	, m_numNodes(0)
	, m_numRenderables(0)
	, m_numTextures(0)
	, m_numMaterials(0)
{
	File file(filename);

	uint8_t *buffer = file.m_buffer;

	bool bLoaded = false;
	uint8_t *ptr = (uint8_t *)(buffer + sizeof(ModelHeader));
	while (!bLoaded)
	{
		ChunkId *chunk = (ChunkId *)ptr;
		if (!strncmp(chunk->tag, "TEXT", 4))
		{
//			LoadTextureChunk(chunk);
		}
		else if (!strncmp(chunk->tag, "MATL", 4))
		{
//			LoadMaterialChunk(chunk);
		}
		else if (!strncmp(chunk->tag, "MESH", 4))
		{
			processMeshChunk(chunk);
			bLoaded = true;
		}
		else
		{
//			AssertMsg(0, "Invalid Chunk");
		}
		ptr = ptr + chunk->size;
	}
}

Mesh::~Mesh()
{
	free(m_renderables);
	free(m_transforms);
	free(m_hierarchy);
	m_renderables = 0;
	m_transforms = 0;
	m_hierarchy = 0;
}

void Mesh::processMeshChunk(ChunkId *chunk)
{

	MeshChunk *meshChunk = (MeshChunk *)chunk;
	uint8_t *ptr = (uint8_t *)chunk + sizeof(MeshChunk);
	m_numNodes = meshChunk->numMeshes;
	m_hierarchy = static_cast<MeshNode *>(malloc(sizeof(MeshNode) * m_numNodes));
	m_transforms = static_cast<glm::mat4x4 *>(malloc(sizeof(glm::mat4x4) * m_numNodes));
	MeshInfo *info = (MeshInfo *)ptr;
	m_numRenderables = info->numRenderables;;
	m_renderables = static_cast<Renderable *>(malloc(sizeof(Renderable) * m_numRenderables));
	uint32_t renderableIndex = 0;
	uint32_t nodeIndex = 0;
	int32_t parentIndex = -1;
	for (uint32_t i = 0; i < meshChunk->numMeshes; ++i)
	{
//		ptr = LoadMeshChunkRecursive(ptr, renderableIndex, nodeIndex, parentIndex, vertexBuffer, vertexBufferOffset, indexBuffer, indexBufferOffset);
	}
}

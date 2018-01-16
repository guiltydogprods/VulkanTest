#include "stdafx.h"
#include "Mesh.h"
#include "Framework/File.h"
#include "Vulkan/RenderDeviceVk.h"

Mesh::Mesh(const char *filename, Buffer& vertexBuffer, int64_t& vertexBufferOffset, Buffer& indexBuffer, int64_t& indexBufferOffset)
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
			processMeshChunk(chunk, vertexBuffer, vertexBufferOffset, indexBuffer, indexBufferOffset);
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

void Mesh::processMeshChunk(ChunkId *chunk, Buffer& vertexBuffer, int64_t& vertexBufferOffset, Buffer& indexBuffer, int64_t& indexBufferOffset)
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
		processMeshRecursive(ptr, renderableIndex, nodeIndex, parentIndex, vertexBuffer, vertexBufferOffset, indexBuffer, indexBufferOffset);
	}
}

uint8_t* Mesh::processMeshRecursive(uint8_t* ptr, uint32_t& renderableIndex, uint32_t& nodeIndex, int32_t parentIndex, Buffer& vertexBuffer, int64_t& vertexBufferOffset, Buffer& indexBuffer, int64_t& indexBufferOffset)
{
	MeshInfo *info = (MeshInfo *)ptr;
	RenderableInfo* rendInfo = (RenderableInfo*)(ptr + sizeof(MeshInfo));

	m_hierarchy[nodeIndex].m_parentId = parentIndex;
	m_hierarchy[nodeIndex].m_numRenderables = info->numRenderables;
	m_hierarchy[nodeIndex].m_numChildren = info->numChildren;
	uint32_t numRenderables = info->numRenderables;

	uint8_t* meshData = (uint8_t*)((uint8_t*)rendInfo + sizeof(RenderableInfo) * numRenderables);
	for (uint32_t rend = 0; rend < numRenderables; ++rend)
	{
		Import::VertexBuffer* srcVertexBuffer = (Import::VertexBuffer*)meshData;
		uint32_t numVertices = rendInfo[rend].numVertices;
		uint32_t stride = srcVertexBuffer->m_streams[0].m_stride;
		uint32_t verticesSize = sizeof(Import::VertexBuffer) + stride * numVertices;
		uint32_t numIndices = rendInfo[rend].numIndices;
		uint32_t indicesSize = sizeof(uint32_t) * numIndices;

		m_aabbMin[0] = rendInfo->aabbMinX;
		m_aabbMin[1] = rendInfo->aabbMinY;
		m_aabbMin[2] = rendInfo->aabbMinZ;
		m_aabbMax[0] = rendInfo->aabbMaxX;
		m_aabbMax[1] = rendInfo->aabbMaxY;
		m_aabbMax[2] = rendInfo->aabbMaxZ;

		int32_t materialIndex = 0;	// FindMaterial(rendInfo->materialHash);
		uint32_t firstVertex = (uint32_t)(vertexBufferOffset / stride);
		uint32_t firstIndex = (uint32_t)(indexBufferOffset / sizeof(uint32_t));
		uint32_t indexCount = numIndices;
		Renderable *renderablePtr = m_renderables + renderableIndex;
		Renderable *pRenderable = new (renderablePtr) Renderable(firstVertex, firstIndex, indexCount, materialIndex);

		//			MaterialPtr pMaterial = MaterialManager::Get()->Find(rendInfo[rend].materialHash);
		uint32_t numStreams = srcVertexBuffer->m_numStreams;

		//			OddJob::dispatchAsync(OddJob::getMainQueue(), [=, &vertexBuffer, &indexBuffer]() {
		for (uint32_t i = 0; i < numStreams; ++i)
		{
			uint8_t* vertexData = (uint8_t*)srcVertexBuffer + sizeof(Import::VertexBuffer);
			Buffer vertexStagingBuffer(Application::GetApplication()->getRenderDevice(), numVertices * stride, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uint8_t *destPtr = static_cast<uint8_t *>(vertexStagingBuffer.mapMemory(0, VK_WHOLE_SIZE));
			if (destPtr)
			{
				memcpy(destPtr, vertexData, numVertices * stride);
				vertexStagingBuffer.unmapMemory();
			}
//			vertexBuffer.writeData(i, vertexBufferOffset, numVertices * stride, vertexData);
		}
		uint32_t* indices = (uint32_t*)((uint8_t*)srcVertexBuffer + verticesSize);
		Buffer indexStagingBuffer(Application::GetApplication()->getRenderDevice(), numIndices * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uint8_t *destPtr = static_cast<uint8_t *>(indexStagingBuffer.mapMemory(0, VK_WHOLE_SIZE));
		if (destPtr)
		{
			memcpy(destPtr, indices, numIndices * sizeof(uint32_t));
			indexStagingBuffer.unmapMemory();
		}
//		indexBuffer.writeData(indexBufferOffset, numIndices * sizeof(uint32_t), indices);
		//			});
		vertexBufferOffset += (numVertices * stride);
		indexBufferOffset += (numIndices * sizeof(uint32_t));
		meshData = meshData + verticesSize + indicesSize;
		renderableIndex++;
	}
	ptr = (uint8_t*)meshData;
	//		(uint32_t*)&info->worldMatrix, 16;
	memcpy(&m_transforms[nodeIndex], &info->worldMatrix, sizeof(glm::mat4x4));
	parentIndex = nodeIndex++;
	uint32_t numChildren = info->numChildren;
	for (uint32_t i = 0; i < numChildren; ++i) {
		ptr = processMeshRecursive(ptr, renderableIndex, nodeIndex, parentIndex, vertexBuffer, vertexBufferOffset, indexBuffer, indexBufferOffset);
		//			renderableIndex++;
	}
	return ptr;
}

Renderable::Renderable(uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount, uint32_t materialIndex)
	: m_firstVertex(firstVertex)
	, m_firstIndex(firstIndex)
	, m_indexCount(indexCount)
	, m_materialIndex(materialIndex)
{
}

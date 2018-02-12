#include "stdafx.h"
#include "File.h"
#include "Mesh.h"
#include "../Vulkan/BufferVk.h"
#include "../Vulkan/RenderDeviceVk.h"

Mesh::Mesh(ScopeStack& scopeStack, RenderDevice& renderDevice, const char *filename, int64_t& vertexBufferOffset, int64_t& indexBufferOffset)
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

	ModelHeader header = {};
	file.readBytes(sizeof(ModelHeader), &header);

	bool bLoaded = false;
	while (!bLoaded)
	{
		ChunkId chunk = {};
		size_t bytesRead = file.readBytes(sizeof(ChunkId), &chunk);
		AssertMsg((bytesRead == sizeof(ChunkId)), "File read failure.\n");
		if (!strncmp(chunk.tag, "TEXT", 4))
		{
//			LoadTextureChunk(chunk);
			file.skipBytes(chunk.size - sizeof(ChunkId));
		}
		else if (!strncmp(chunk.tag, "MATL", 4))
		{
//			LoadMaterialChunk(chunk);
			file.skipBytes(chunk.size - sizeof(ChunkId));
		}
		else if (!strncmp(chunk.tag, "MESH", 4))
		{
			processMeshChunk(scopeStack, file, renderDevice, vertexBufferOffset, indexBufferOffset);
			bLoaded = true;
		}
		else
		{
//			AssertMsg(0, "Invalid Chunk");
		}
	}
}

Mesh::~Mesh()
{
}

void Mesh::processMeshChunk(ScopeStack& scopeStack, File& file, RenderDevice& renderDevice, int64_t& vertexBufferOffset, int64_t& indexBufferOffset)
{
	MeshChunk meshChunk = {};
	file.rewindBytes(sizeof(ChunkId));
	size_t bytesRead = file.readBytes(sizeof(meshChunk), &meshChunk);
	AssertMsg((bytesRead == sizeof(meshChunk)), "File read failure.\n");

	m_numNodes = meshChunk.numMeshes;
	m_hierarchy = static_cast<MeshNode *>(scopeStack.allocate(sizeof(MeshNode) * m_numNodes));
	m_transforms = static_cast<glm::mat4x4 *>(scopeStack.allocate(sizeof(glm::mat4x4) * m_numNodes));
	MeshInfo meshInfo = {};
	bytesRead = file.readBytes(sizeof(meshInfo), &meshInfo);
	AssertMsg((bytesRead == sizeof(meshInfo)), "File read failure.\n");
	m_numRenderables = meshInfo.numRenderables;
	m_renderables = static_cast<Renderable *>(scopeStack.allocate(sizeof(Renderable) * m_numRenderables));
	uint64_t location = file.rewindBytes(sizeof(MeshInfo));

// We have allocated all the permanent data  we need, so we can read the remainder of the file into a temporary buffer.
	ScopeStack tempScope(scopeStack, "Temp");
	uint64_t bytesToRead = file.m_sizeInBytes - location;
	uint8_t *ptr = static_cast<uint8_t *>(tempScope.allocate(bytesToRead));
	file.readBytes(bytesToRead, ptr);
	AssertMsg((bytesRead == sizeof(meshInfo)), "File read failure.\n");

	uint32_t renderableIndex = 0;
	uint32_t nodeIndex = 0;
	int32_t parentIndex = -1;
	for (uint32_t i = 0; i < meshChunk.numMeshes; ++i)
	{
		processMeshRecursive(ptr, renderableIndex, nodeIndex, parentIndex, renderDevice, vertexBufferOffset, indexBufferOffset);
	}
}

uint8_t* Mesh::processMeshRecursive(uint8_t* ptr, uint32_t& renderableIndex, uint32_t& nodeIndex, int32_t parentIndex, RenderDevice& renderDevice, int64_t& vertexBufferOffset, int64_t& indexBufferOffset)
{
	Buffer& vertexBuffer = *renderDevice.m_vertexBuffer;
	Buffer& indexBuffer = *renderDevice.m_indexBuffer;

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

		uint32_t numStreams = srcVertexBuffer->m_numStreams;

		for (uint32_t i = 0; i < numStreams; ++i)
		{
			VkCommandBuffer copyCommandBuffer = renderDevice.beginSingleUseCommandBuffer();
			VkBufferCopy copyRegion = {};

			uint8_t* vertexData = (uint8_t*)srcVertexBuffer + sizeof(Import::VertexBuffer);
			StagingBuffer vertexStagingBuffer(renderDevice, numVertices * stride, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uint8_t *destPtr = static_cast<uint8_t *>(vertexStagingBuffer.mapMemory(0, VK_WHOLE_SIZE));
			if (destPtr)
			{
				memcpy(destPtr, vertexData, numVertices * stride);
				vertexStagingBuffer.unmapMemory();
				vertexStagingBuffer.bindMemory();
			}
			copyRegion.size = numVertices * stride;
			copyRegion.dstOffset = vertexBufferOffset;
			vkCmdCopyBuffer(copyCommandBuffer, vertexStagingBuffer.m_buffer, vertexBuffer.m_buffer, 1, &copyRegion);

			renderDevice.endSingleUseCommandBuffer(copyCommandBuffer);
		}

		VkCommandBuffer copyCommandBuffer = renderDevice.beginSingleUseCommandBuffer();
		VkBufferCopy copyRegion = {};

		uint32_t* indices = (uint32_t*)((uint8_t*)srcVertexBuffer + verticesSize);
		StagingBuffer indexStagingBuffer(renderDevice, numIndices * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uint8_t *destPtr = static_cast<uint8_t *>(indexStagingBuffer.mapMemory(0, VK_WHOLE_SIZE));
		if (destPtr)
		{
			memcpy(destPtr, indices, numIndices * sizeof(uint32_t));
			indexStagingBuffer.unmapMemory();
			indexStagingBuffer.bindMemory();
		}
		copyRegion.dstOffset = indexBufferOffset;
		copyRegion.size = numIndices * sizeof(uint32_t);
		vkCmdCopyBuffer(copyCommandBuffer, indexStagingBuffer.m_buffer, indexBuffer.m_buffer, 1, &copyRegion);
		renderDevice.endSingleUseCommandBuffer(copyCommandBuffer);

		vertexBufferOffset += (numVertices * stride);
		indexBufferOffset += (numIndices * sizeof(uint32_t));
		meshData = meshData + verticesSize + indicesSize;
		renderableIndex++;
	}

	ptr = (uint8_t*)meshData;
	memcpy(&m_transforms[nodeIndex], &info->worldMatrix, sizeof(glm::mat4x4));
	parentIndex = nodeIndex++;
	uint32_t numChildren = info->numChildren;
	for (uint32_t i = 0; i < numChildren; ++i)
	{
		ptr = processMeshRecursive(ptr, renderableIndex, nodeIndex, parentIndex, renderDevice, vertexBufferOffset, indexBufferOffset);
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

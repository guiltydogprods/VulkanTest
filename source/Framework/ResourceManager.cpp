#include "stdafx.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "../Vulkan/TextureVk.h"

const uint32_t kMaxFilenameLength = 64;

void ResourceManager::registerResources(ScopeStack&scope, RenderDevice& renderDevice, uint32_t numResources, ResourceName *resources)
{
	uint32_t numMeshes = 0;
	uint32_t numTextures = 0;
	for (uint32_t i = 0; i < numResources; ++i)
	{
		switch (resources[i].resourceType)
		{
		case kRTMesh:
			numMeshes++;
			break;
		case kRTTexture:
			numTextures++;
			break;
		default:
			break;
		}
	}

	m_meshes = static_cast<Mesh **>(scope.allocate(sizeof(Mesh *) * numMeshes));
	m_numMeshes = numMeshes;
	m_textures = static_cast<Texture **>(scope.allocate(sizeof(Texture *) * numTextures));
	m_numTextures = numTextures;

	uint32_t meshIndex = 0;
	uint32_t textureIndex = 0;
	int64_t vertexBufferOffset = 0;
	int64_t indexBufferOffset = 0;
	for (uint32_t i = 0; i < numResources; ++i)
	{
		switch (resources[i].resourceType)
		{
		case kRTMesh:
			m_meshes[meshIndex++] = static_cast<Mesh *>(scope.newObject<Mesh>(scope, renderDevice, resources[i].resourceFilename, vertexBufferOffset, indexBufferOffset));
			break;
		case kRTTexture:
			m_textures[textureIndex++] = static_cast<Texture *>(scope.newObject<Texture>(scope, renderDevice, resources[i].resourceFilename));
			break;
		default:
			break;
		}
	}
}

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

void ResourceManager::registerResource(ScopeStack& scope, ResourceName& resource)
{
	void *addr = scope.newPOD<ResourceEntry>();
	ResourceEntry *entry = new (addr) ResourceEntry(scope, resource.resourceFilename, resource.resourceType);
	if (m_resourceCount++ == 0)
		m_resourceList = entry;

	switch (resource.resourceType)
	{
	case kRTMesh:
		m_numMeshes++;
		Mesh::RegisterSubresources(scope, *this, resource.resourceFilename);
		break;
	case kRTTexture:
		m_numTextures++;
		break;
	default:
		break;
	}
}

void ResourceManager::loadResources(ScopeStack&scope, RenderDevice& renderDevice)
{
	m_meshes = static_cast<Mesh **>(scope.allocate(sizeof(Mesh *) * m_numMeshes));
	m_textures = static_cast<Texture **>(scope.allocate(sizeof(Texture *) * m_numTextures));

	uint32_t meshIndex = 0;
	uint32_t textureIndex = 0;
	int64_t vertexBufferOffset = 0;
	int64_t indexBufferOffset = 0;
	ResourceEntry *resourceEntry = m_resourceList;
	for (uint32_t i = 0; i < m_resourceCount; ++i)
	{
		const char *filename = resourceEntry->resourceFilename.filename;
		switch (resourceEntry->resourceType)
		{
		case kRTMesh:
			m_meshes[meshIndex++] = static_cast<Mesh *>(scope.newObject<Mesh>(scope, renderDevice, filename, vertexBufferOffset, indexBufferOffset));
			break;
		case kRTTexture:
			m_textures[textureIndex++] = static_cast<Texture *>(scope.newObject<Texture>(scope, renderDevice, filename));
			break;
		default:
			break;
		}
		resourceEntry++;
	}
}


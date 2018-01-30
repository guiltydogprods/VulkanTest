#pragma once 

struct Buffer;
struct Mesh;
struct MemoryBlock;
struct MemorySubBlock;

struct MemAllocInfo
{
	VkDeviceMemory memoryBlock;
	VkDeviceSize offset;
};

const uint32_t kMaxBlocks = 16;

struct RenderDevice
{
	RenderDevice();
	~RenderDevice();

	struct MemoryManager
	{
		friend struct RenderDevice;
	public:
		static MemoryManager& Instance();
		MemAllocInfo allocate(VkDeviceSize size, VkDeviceSize alignment, uint32_t typeIndex);
		MemoryBlock& findBlock(VkDeviceSize size, VkDeviceSize alignment, uint32_t typeIndex);


		RenderDevice *m_pRenderDevice;
		MemoryBlock *m_blocks[kMaxBlocks];
		uint32_t m_numBlocks;
	private:
		inline void setRenderDevice(RenderDevice *pRenderDevice) { m_pRenderDevice = pRenderDevice; }

		MemoryManager();
	};

	void initialize(GLFWwindow *window);
	void finalize(Mesh *meshes, uint32_t numMeshes);
	void cleanup();
	void update();
	void render();

	void cleanupSwapChain();
	void createInstance();
	void createSurface(GLFWwindow *window);
	void createDevice();
	void createSemaphores();
	void createCommandPool();
	void createDepthBuffer();
	void createTexture(const char *filename);
	void createVertexBuffer();
	void createUniformBuffer();
	void createSwapChain();
	void recreateSwapChain();
	void createRenderPass();
	void createFramebuffers();
	void createGraphicsPipeline();
	void createDescriptorSet();
	void createCommandBuffers(Mesh *meshes, uint32_t numMeshes);

	int32_t getMemoryType(uint32_t typeBits, VkFlags properties);	//, uint32_t& typeIndex);
	MemAllocInfo allocateGpuMemory(VkDeviceSize size, VkDeviceSize alignment, uint32_t typeIndex);
	VkShaderModule createShaderModule(const char *filename);
	VkCommandBuffer beginSingleUseCommandBuffer();
	void endSingleUseCommandBuffer(VkCommandBuffer commandBuffer);
	void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageSubresourceRange& subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height);

	uint32_t							m_selectedDevice;
	VkInstance							m_vkInstance;

	VkDebugReportCallbackEXT			m_vkDebugReportCallback;

	uint32_t							m_vkPhysicalDeviceCount;
	VkPhysicalDevice					*m_vkPhysicalDevices;
	VkPhysicalDeviceProperties			*m_vkPhysicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties	*m_vkPhysicalDeviceMemoryProperties;
	VkPhysicalDeviceFeatures			*m_vkPhysicalDeviceFeatures;

	uint32_t							m_vkQueueFamilyPropertiesCount;
	VkQueueFamilyProperties				*m_vkQueueFamilyProperties;

	uint32_t							m_vkGraphicsQueueFamilyIndex;
	uint32_t							m_vkPresentQueueFamilyIndex;

	VkDevice							m_vkDevice;

	VkQueue								m_vkGraphicsQueue;
	VkQueue								m_vkPresentQueue;

	VkSurfaceKHR						m_vkSurface;
	VkSemaphore							m_vkImageAvailableSemaphore;
	VkSemaphore							m_vkRenderingFinishedSemaphore;

	Buffer								*m_vertexBuffer;
	Buffer								*m_indexBuffer;

	VkVertexInputBindingDescription		m_vkVertexBindingDescription;
	uint32_t							m_vkVertexAttributeDescriptionCount;
	VkVertexInputAttributeDescription	*m_vkVertexAttributeDescriptions;

	VkBuffer							m_vkUniformBuffer;
//	VkDeviceMemory						m_vkUniformBufferMemory;
	MemAllocInfo						m_uniformBufferMemAllocInfo;

	VkExtent2D							m_vkSwapChainExtent;
	VkFormat							m_vkSwapChainFormat;
	VkSwapchainKHR						m_vkSwapChain;
	uint32_t							m_vkSwapChainImageCount;
	VkImage								*m_vkSwapChainImages;
	VkImageView							*m_vkSwapChainImageViews;
	VkFramebuffer						*m_vkSwapChainFramebuffers;

	VkRenderPass						m_vkRenderPass;

	VkDescriptorSetLayout				m_vkDescriptorSetLayout;
	VkPipelineLayout					m_vkPipelineLayout;
	VkPipeline							m_vkGraphicsPipeline;

	VkDescriptorPool					m_vkDescriptorPool;
	VkDescriptorSet						m_vkDescriptorSet;

	VkCommandPool						m_vkCommandPool;
	VkCommandBuffer						*m_vkCommandBuffers;

	VkFormat							m_vkDepthBufferFormat;
	VkImage								m_vkDepthBufferImage;
	VkImageView							m_vkDepthBufferView;
	MemAllocInfo						m_depthBufferMemAllocInfo;

	VkImage								m_vkTextureImage[2];
//	VkDeviceMemory						m_vkTextureImageMemory[2];
	MemAllocInfo						m_textureMemAllocInfo[2];
	VkImageView							m_vkTextureImageView[2];
	VkSampler							m_vkSampler;
	uint32_t							m_numTextures;

	Mesh								*m_meshes;
	uint32_t							m_numMeshes;
};

struct MemoryBlock
{
	MemoryBlock(VkDevice device, VkDeviceSize size, uint32_t typeIndex);
	~MemoryBlock();

	MemAllocInfo allocate(VkDeviceSize size, VkDeviceSize alignment);

	VkDevice m_vkDevice;
	VkDeviceMemory m_memory;
	VkDeviceSize m_size;
	VkDeviceSize m_offset;
	uint32_t m_typeIndex;
};

struct Buffer
{
	Buffer(RenderDevice& renderDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
	virtual ~Buffer();

	virtual void bindMemory();

protected:
	Buffer(RenderDevice& renderDevice)
		: m_renderDevice(renderDevice)
		, m_buffer(nullptr)
		, m_memAllocInfo{ nullptr, 0 }
		, m_allocatedSize(0) {}

public:
	RenderDevice&	m_renderDevice;
	VkBuffer		m_buffer;
	MemAllocInfo    m_memAllocInfo;
	VkDeviceSize	m_allocatedSize;
};

struct StagingBuffer : public Buffer
{
	StagingBuffer(RenderDevice& renderDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
	~StagingBuffer();

	void *mapMemory(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
	void unmapMemory();
};


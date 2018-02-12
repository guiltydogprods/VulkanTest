#pragma once 

struct Buffer;
struct Mesh;
struct GPUMemAllocInfo;
struct GPUMemoryBlock;
struct MemorySubBlock;
struct Texture;

const uint32_t kMaxGPUMemoryBlocks = 16;
const uint32_t kGPUMemoryBlockSize = 256 * 1024 * 1024;

struct GPUMemoryBlock
{
	GPUMemoryBlock(VkDevice device, uint32_t size, uint32_t typeIndex);
	GPUMemoryBlock() {};
	~GPUMemoryBlock();

	GPUMemAllocInfo& allocate(ScopeStack& scope, uint32_t size, uint32_t alignment, uint32_t optimal);

	VkDevice m_vkDevice;
	VkDeviceMemory m_memory;
	uint32_t m_size;
	uint32_t m_offset;
	uint32_t m_typeIndex;
	uint32_t m_optimal;
};

struct GPUMemAllocInfo
{
	GPUMemAllocInfo(GPUMemoryBlock& _memoryBlock, uint32_t _size, uint32_t _offset)
		: memoryBlock(_memoryBlock), size(_size), offset(_offset) {}

	~GPUMemAllocInfo() { memoryBlock.m_offset = offset; }

	GPUMemoryBlock& memoryBlock;
	uint32_t size;
	uint32_t offset;
};

static GPUMemoryBlock _dummyMemoryBlock = {};
static GPUMemAllocInfo _dummyMemAllocInfo = { _dummyMemoryBlock, 0, 0 };

struct RenderDevice
{
	RenderDevice(ScopeStack& scope, uint32_t maxWidth, uint32_t maxHeight, GLFWwindow* window = nullptr);
	~RenderDevice();

	struct GPUMemoryManager
	{
		friend struct RenderDevice;
	public:
		static GPUMemoryManager& Instance(RenderDevice *renderDevice = nullptr);
		GPUMemAllocInfo& allocate(ScopeStack& scope, VkDeviceSize size, VkDeviceSize alignment, uint32_t typeIndex);
		GPUMemoryBlock& findBlock(ScopeStack& scope, VkDeviceSize size, VkDeviceSize alignment, uint32_t typeIndex);


		RenderDevice& m_renderDevice;
		GPUMemoryBlock *m_blocks[kMaxGPUMemoryBlocks];
		uint32_t m_numBlocks;
		uint32_t m_bufferImageGranularity;
	private:
		GPUMemoryManager(RenderDevice& renderDevice);
	};

	void initialize(ScopeStack& scope, GLFWwindow *window);
	void finalize(ScopeStack& scope, Mesh **meshes, uint32_t numMeshes, Texture **textures, uint32_t numTextures);
	void cleanup();
	void update();
	void render(ScopeStack& scope);

	void cleanupSwapChain();
	void createInstance();
	void createSurface(GLFWwindow *window);
	void createDevice(ScopeStack& scope);
	void createSemaphores();
	void createCommandPool();
	void createDepthBuffer(ScopeStack& scope);
	void createVertexFormat();
	void createUniformBuffer(ScopeStack& scope);
	void createSwapChain(ScopeStack& scope);
	void createSwapChain(ScopeStack* scope = nullptr);
	void createRenderPass();
	void createFramebuffers();
	void createGraphicsPipeline(ScopeStack& scope);
	void createDescriptorSet();
	void createCommandBuffers(Mesh **meshes, uint32_t numMeshes);
	void recreateSwapChain(ScopeStack& scope);
	void recreateDepthBuffer();

	int32_t getMemoryType(uint32_t typeBits, VkFlags properties);
	VkDeviceMemory allocateGpuMemory(VkDeviceSize size, VkDeviceSize alignment, uint32_t typeIndex);
	VkShaderModule createShaderModule(ScopeStack& scope, const char *filename);
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
	Buffer								*m_uniformBuffer;

	VkVertexInputBindingDescription		m_vkVertexBindingDescription;
	uint32_t							m_vkVertexAttributeDescriptionCount;
	VkVertexInputAttributeDescription	*m_vkVertexAttributeDescriptions;

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
	std::reference_wrapper<GPUMemAllocInfo> m_depthBufferMemAllocInfo;

	Mesh								**m_meshes;
	Texture								**m_textures;
	uint32_t							m_numMeshes;
	uint32_t							m_numTextures;


	uint32_t							m_maxWidth;
	uint32_t							m_maxHeight;
};

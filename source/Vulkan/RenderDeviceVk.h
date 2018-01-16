#pragma once 

struct Buffer;

struct RenderDevice
{
	RenderDevice();
	~RenderDevice();

	void initialize(GLFWwindow *window);
	void finalize();
	void cleanup();
	void update();
	void render();

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
	void createRenderPass();
	void createFramebuffers();
	void createGraphicsPipeline();
	void createDescriptorSet();
	void createCommandBuffers();

	VkBool32 getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t& typeIndex);

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
	VkDeviceMemory						m_vkUniformBufferMemory;

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
	VkDeviceMemory						m_vkDepthBufferMemory;
	VkImageView							m_vkDepthBufferView;

	VkImage								m_vkTextureImage;
	VkDeviceMemory						m_vkTextureImageMemory;
	VkImageView							m_vkTextureImageView;
	VkSampler							m_vkSampler;
};

struct Buffer
{
	Buffer(RenderDevice& renderDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
	~Buffer();

	void *mapMemory(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
	void unmapMemory();
	void bindMemory();

	VkDevice&		m_vkDevice;
	VkBuffer		m_buffer;
	VkDeviceMemory	m_memory;
	VkDeviceSize	m_allocatedSize;
};

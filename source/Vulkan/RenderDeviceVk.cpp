#include "stdafx.h"
#include "RenderDeviceVk.h"
#include "BufferVk.h"
#include "RenderTargetVk.h"
#include "SceneVk.h"
#include "TextureVk.h"

#include "../Framework/Mesh.h"

//#define FORCE_NVIDIA
//#define FORCE_INTEL

const char* validationLayers[] = 
{
	"VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
const bool kEnableValidationLayers = false;
#else
const bool kEnableValidationLayers = true;
#endif

const uint64_t  kDefaultFenceTimeout = 100000000000;

struct Vertex
{
	float pos[3];
	float color[3];
	float texCoord[2];
};

struct ModelUniformData
{
	glm::mat4x4 tranformationMatrix[3];
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		print("ERROR: validation layer: %s\n", msg);
	else
		print("WARNING: validation layer: %s\n", msg);
	return VK_FALSE;
}

RenderDevice::RenderDevice(ScopeStack& scope, uint32_t maxWidth, uint32_t maxHeight, GLFWwindow* window)
	: m_selectedDevice(0)
	, m_vkInstance(nullptr)
	, m_vkPhysicalDeviceCount(0)
	, m_vkPhysicalDevices(nullptr)
	, m_vkPhysicalDeviceProperties(nullptr)
	, m_vkPhysicalDeviceMemoryProperties(nullptr)
	, m_vkPhysicalDeviceFeatures(nullptr)
	, m_vkQueueFamilyPropertiesCount(0)
	, m_vkQueueFamilyProperties(nullptr)
	, m_vkGraphicsQueueFamilyIndex(0)
	, m_vkPresentQueueFamilyIndex(0)
	, m_vertexBuffer(nullptr)
	, m_indexBuffer(nullptr)
	, m_vkVertexAttributeDescriptionCount(0)
	, m_vkVertexAttributeDescriptions(nullptr)
	, m_vkSwapChainImageCount(0)
	, m_swapChainRenderTargets(nullptr)
	, m_vkSwapChainFramebuffers(nullptr)
	, m_vkCommandBuffers(nullptr)
	, m_depthRenderTarget(nullptr)
	, m_aaRenderTarget(nullptr)
	, m_aaDepthRenderTarget(nullptr)
	, m_scene(nullptr)
	, m_textures(nullptr)
	, m_numTextures(0)
	, m_maxWidth(maxWidth)
	, m_maxHeight(maxHeight)
{
	createInstance();
	createSurface(window);
	createDevice(scope);

	GPUMemoryManager& memMgr = GPUMemoryManager::Instance(this);
	memMgr.m_bufferImageGranularity = static_cast<uint32_t>(m_vkPhysicalDeviceProperties[m_selectedDevice].limits.bufferImageGranularity);
	initialize(scope, window);
}

RenderDevice::~RenderDevice()
{
	cleanup();
}

void RenderDevice::initialize(ScopeStack& scope, GLFWwindow *window)
{
	createSemaphores();
	createSwapChain(scope);	
	createCommandPool();
//	m_depthRendereTarget = scope.newObject<RenderTarget>(scope, *this, m_maxWidth, m_maxHeight, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
	m_aaRenderTarget = scope.newObject<RenderTarget>(scope, *this, m_maxWidth, m_maxHeight, m_vkSwapChainFormat, VK_SAMPLE_COUNT_4_BIT);
	m_aaDepthRenderTarget = scope.newObject<RenderTarget>(scope, *this, m_maxWidth, m_maxHeight, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_4_BIT);
	m_dummyTexture = scope.newObject<Texture>(scope, *this, 1, 1, VK_FORMAT_R32_UINT);
}

void RenderDevice::finalize(ScopeStack& scope, Scene& scene, Texture **textures, uint32_t numTextures)
{
	m_scene = &scene;
	m_textures = textures;
	m_numTextures = numTextures;
	createRenderPass();
	createFramebuffers();
	createGraphicsPipeline(scope);
	createDescriptorSet(scene);
	createCommandBuffers(scene);
}

void RenderDevice::cleanupSwapChain()
{
	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, m_vkSwapChainImageCount, m_vkCommandBuffers);

	for (uint32_t i = 0; i < m_vkSwapChainImageCount; ++i)
		vkDestroyFramebuffer(m_vkDevice, m_vkSwapChainFramebuffers[i], nullptr);

	vkDestroyPipeline(m_vkDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyDescriptorSetLayout(m_vkDevice, m_vkDescriptorSetLayout, nullptr);
	vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);

	vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
}

void RenderDevice::cleanup()
{
	vkDeviceWaitIdle(m_vkDevice);
	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, m_vkSwapChainImageCount, m_vkCommandBuffers);
	vkDestroyDescriptorSetLayout(m_vkDevice, m_vkDescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr);
	vkDestroyPipeline(m_vkDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);

	for (uint32_t i = 0; i < m_vkSwapChainImageCount; ++i)
		vkDestroyFramebuffer(m_vkDevice, m_vkSwapChainFramebuffers[i], nullptr);

	vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
	vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
	vkDestroySemaphore(m_vkDevice, m_vkImageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_vkDevice, m_vkRenderingFinishedSemaphore, nullptr);
	vkDestroyDevice(m_vkDevice, nullptr);
	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	if (kEnableValidationLayers)
	{
		PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugReportCallbackEXT");
		DestroyDebugReportCallback(m_vkInstance, m_vkDebugReportCallback, nullptr);
	}
	vkDestroyInstance(m_vkInstance, nullptr);
}

bool RenderDevice::getBackBufferIndex(ScopeStack& scope, uint32_t& backBufferIndex)
{
	VkResult res = vkAcquireNextImageKHR(m_vkDevice, m_vkSwapChain, UINT64_MAX, m_vkImageAvailableSemaphore, VK_NULL_HANDLE, &backBufferIndex);
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain(scope);
		return false;
	}
	else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
	{
		print("failed to acquire image\n");
		return false;
	}
	return true;
}

VkCommandBuffer RenderDevice::getCommandBuffer(uint32_t backBufferIndex)
{
	return m_vkCommandBuffers[backBufferIndex];
}

void RenderDevice::submit(VkCommandBuffer commandBuffer, VkSemaphore *waitSemaphore, VkSemaphore *signalSemaphore)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	if (waitSemaphore)
	{
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphore;
	}
	if (signalSemaphore)
	{
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_vkRenderingFinishedSemaphore;
	}
	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkFence fence;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (vkCreateFence(m_vkDevice, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS)
	{
		print("failed to create fence.\n");
	}
	VkResult res = vkQueueSubmit(m_vkPresentQueue, 1, &submitInfo, fence);
	if (res != VK_SUCCESS)
	{
		print("failed to submit draw command buffer\n");
		//		exit(EXIT_FAILURE);
	}

	if (vkWaitForFences(m_vkDevice, 1, &fence, VK_TRUE, kDefaultFenceTimeout) != VK_SUCCESS)
	{
		print("failed to wait on fence.\n");
	}
	vkDestroyFence(m_vkDevice, fence, nullptr);
}

void RenderDevice::present(ScopeStack& scope, uint32_t backBufferIndex)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_vkRenderingFinishedSemaphore;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_vkSwapChain;
	presentInfo.pImageIndices = &backBufferIndex;

	VkResult res = vkQueuePresentKHR(m_vkPresentQueue, &presentInfo);
	if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapChain(scope);
	}
	else if (res != VK_SUCCESS)
	{
		print("failed to present swap chain image!");
	}
}

void RenderDevice::createInstance()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	uint32_t extensionCount = glfwExtensionCount + (kEnableValidationLayers ? 1 : 0);
	const char **extensions = static_cast<const char**>(alloca(sizeof(const char*) * extensionCount));
	for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		extensions[i] = glfwExtensions[i];

	if (kEnableValidationLayers)
		extensions[glfwExtensionCount] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;

	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	VkLayerProperties *availableLayers = static_cast<VkLayerProperties *>(alloca(sizeof(VkLayerProperties) * layerCount));
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
	bool validationLayersFound = false;
	for (uint32_t i = 0; i < layerCount; ++i)
	{
		if (!strcmp(availableLayers[i].layerName, validationLayers[0]))
		{
			validationLayersFound = true;
			break;
		}
	}

	Application* app = Application::GetApplication();

	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = app->getApplicationName();
	applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	applicationInfo.pEngineName = "VulkanTest";
	applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledExtensionCount = extensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = extensions;
	if (kEnableValidationLayers && validationLayersFound)
	{
		instanceCreateInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(const char*);
		instanceCreateInfo.ppEnabledLayerNames = validationLayers;
	}
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.ppEnabledLayerNames = nullptr;
	}

	VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance);

	if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		print("cannot find a compatible Vulkan ICD\n");
		exit(-1);
	}
	else if (res)
	{
		print("unknown error\n");
		exit(-1);
	}

	if (kEnableValidationLayers) 
	{
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugCallback;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

		PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugReportCallbackEXT");

		if (CreateDebugReportCallback(m_vkInstance, &createInfo, nullptr, &m_vkDebugReportCallback) != VK_SUCCESS)
		{
			print("failed to create debug callback\n");
			exit(1);
		}
		else
		{
			print("created debug callback\n");
		}
	}
	else 
	{
		print("skipped creating debug callback\n");
	}
}

void RenderDevice::createSurface(GLFWwindow *window)
{
	VkResult err = glfwCreateWindowSurface(m_vkInstance, window, nullptr, &m_vkSurface);
	if (err)
	{
		print("window surface creation failed.\n");
		exit(-1);
	}
}

void RenderDevice::createDevice(ScopeStack& scope)
{
	vkEnumeratePhysicalDevices(m_vkInstance, &m_vkPhysicalDeviceCount, nullptr);

	m_vkPhysicalDevices = static_cast<VkPhysicalDevice *>(scope.allocate(sizeof(VkPhysicalDevice) * m_vkPhysicalDeviceCount));
	vkEnumeratePhysicalDevices(m_vkInstance, &m_vkPhysicalDeviceCount, m_vkPhysicalDevices);
	m_vkPhysicalDeviceProperties = static_cast<VkPhysicalDeviceProperties *>(scope.allocate(sizeof(VkPhysicalDeviceProperties) * m_vkPhysicalDeviceCount));
	m_vkPhysicalDeviceMemoryProperties = static_cast<VkPhysicalDeviceMemoryProperties *>(scope.allocate(sizeof(VkPhysicalDeviceMemoryProperties) * m_vkPhysicalDeviceCount));
	m_vkPhysicalDeviceFeatures = static_cast<VkPhysicalDeviceFeatures *>(scope.allocate(sizeof(VkPhysicalDeviceFeatures) * m_vkPhysicalDeviceCount));
	for (uint32_t i = 0; i < m_vkPhysicalDeviceCount; ++i)
	{
		vkGetPhysicalDeviceProperties(m_vkPhysicalDevices[i], &m_vkPhysicalDeviceProperties[i]);
		vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevices[i], &m_vkPhysicalDeviceMemoryProperties[i]);
		vkGetPhysicalDeviceFeatures(m_vkPhysicalDevices[i], &m_vkPhysicalDeviceFeatures[i]);
	}

#ifdef WIN32
	SYSTEM_POWER_STATUS powerStatus = {};
	if (GetSystemPowerStatus(&powerStatus))
	{
		if (powerStatus.ACLineStatus == 0 && m_vkPhysicalDeviceCount > 1 && m_vkPhysicalDeviceProperties[1].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			m_selectedDevice = 1;				// Automatically select Integrated GPU if available when on battery power. 
	}
#endif
#if defined(FORCE_NVIDIA)
	m_selectedDevice = 0;
#elif defined(FORCE_INTEL)
	m_selectedDevice = 1;
#endif

	print("Selected Device: %s\n", m_vkPhysicalDeviceProperties[m_selectedDevice].deviceName);

	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevices[m_selectedDevice], &m_vkQueueFamilyPropertiesCount, nullptr);
	m_vkQueueFamilyProperties = static_cast<VkQueueFamilyProperties *>(scope.allocate(sizeof(VkQueueFamilyProperties) * m_vkQueueFamilyPropertiesCount));
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevices[m_selectedDevice], &m_vkQueueFamilyPropertiesCount, m_vkQueueFamilyProperties);

	bool foundGraphicsQueueFamily = false;
	bool foundPresentQueueFamily = false;

	for (uint32_t i = 0; i < m_vkQueueFamilyPropertiesCount; i++)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevices[m_selectedDevice], i, m_vkSurface, &presentSupport);

		if (m_vkQueueFamilyProperties[i].queueCount > 0 && m_vkQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_vkGraphicsQueueFamilyIndex = i;
			foundGraphicsQueueFamily = true;

			if (presentSupport)
			{
				m_vkPresentQueueFamilyIndex = i;
				foundPresentQueueFamily = true;
				break;
			}
		}

		if (!foundPresentQueueFamily && presentSupport)
		{
			m_vkPresentQueueFamilyIndex = i;
			foundPresentQueueFamily = true;
		}
	}

	if (foundGraphicsQueueFamily)
	{
		print("queue family #%d supports graphics\n", m_vkGraphicsQueueFamilyIndex);

		if (foundPresentQueueFamily)
		{
			print("queue family #%d supports presentation\n", m_vkPresentQueueFamilyIndex);
		}
		else
		{
			print("could not find a valid queue family with present support\n");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		print("could not find a valid queue family with graphics support\n");
		exit(EXIT_FAILURE);
	}

	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo[2] = {};

	queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[0].pNext = nullptr;
	queueCreateInfo[0].flags = 0;
	queueCreateInfo[0].queueFamilyIndex = m_vkGraphicsQueueFamilyIndex;
	queueCreateInfo[0].queueCount = 1;
	queueCreateInfo[0].pQueuePriorities = &queuePriority;

	queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[1].pNext = nullptr;
	queueCreateInfo[1].flags = 0;
	queueCreateInfo[1].queueFamilyIndex = m_vkPresentQueueFamilyIndex;
	queueCreateInfo[1].queueCount = 1;
	queueCreateInfo[1].pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = m_vkGraphicsQueueFamilyIndex != m_vkPresentQueueFamilyIndex ? 2 : 1;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions;
	deviceCreateInfo.pEnabledFeatures = &m_vkPhysicalDeviceFeatures[m_selectedDevice];

	VkResult res = vkCreateDevice(m_vkPhysicalDevices[m_selectedDevice], &deviceCreateInfo, nullptr, &m_vkDevice);

	vkGetDeviceQueue(m_vkDevice, m_vkGraphicsQueueFamilyIndex, 0, &m_vkGraphicsQueue);
	vkGetDeviceQueue(m_vkDevice, m_vkPresentQueueFamilyIndex, 0, &m_vkPresentQueue);
	print("acquired graphics and presentation queues\n");
}

void RenderDevice::createSemaphores() 
{
	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(m_vkDevice, &createInfo, nullptr, &m_vkImageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_vkDevice, &createInfo, nullptr, &m_vkRenderingFinishedSemaphore) != VK_SUCCESS)
	{
		print("failed to create semaphores\n");
		exit(EXIT_FAILURE);
	}
	else 
	{
		print("created semaphores\n");
	}
}

void RenderDevice::createCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = m_vkPresentQueueFamilyIndex;

	if (vkCreateCommandPool(m_vkDevice, &commandPoolCreateInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS)
	{
		print("failed to create command pool\n");
		exit(EXIT_FAILURE);
	}
}

void RenderDevice::createVertexFormat(ScopeStack& scope)
{
	m_vkVertexBindingDescription.binding = 0;
	m_vkVertexBindingDescription.stride = sizeof(Vertex);
	m_vkVertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	m_vkVertexAttributeDescriptionCount = 3;
	m_vkVertexAttributeDescriptions = static_cast<VkVertexInputAttributeDescription *>(scope.allocate(sizeof(VkVertexInputAttributeDescription) * m_vkVertexAttributeDescriptionCount));
	m_vkVertexAttributeDescriptions[0].binding = 0;
	m_vkVertexAttributeDescriptions[0].location = 0;
	m_vkVertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vkVertexAttributeDescriptions[0].offset = 0;

	m_vkVertexAttributeDescriptions[1].binding = 0;
	m_vkVertexAttributeDescriptions[1].location = 1;
	m_vkVertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vkVertexAttributeDescriptions[1].offset = sizeof(float) * 3;

	m_vkVertexAttributeDescriptions[2].binding = 0;
	m_vkVertexAttributeDescriptions[2].location = 2;
	m_vkVertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	m_vkVertexAttributeDescriptions[2].offset = sizeof(float) * 6;
}

void RenderDevice::createSwapChain(ScopeStack& scope)
{
	createSwapChain(&scope);
}

void RenderDevice::createSwapChain(ScopeStack *scope)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevices[m_selectedDevice], m_vkSurface, &surfaceCapabilities);

	uint32_t supportedSurfaceFormatsCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevices[m_selectedDevice], m_vkSurface, &supportedSurfaceFormatsCount, nullptr);
	VkSurfaceFormatKHR *supportedSurfaceFormats = static_cast<VkSurfaceFormatKHR *>(alloca(sizeof(VkSurfaceCapabilitiesKHR) * supportedSurfaceFormatsCount));
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevices[m_selectedDevice], m_vkSurface, &supportedSurfaceFormatsCount, supportedSurfaceFormats);

	m_vkSwapChainExtent = surfaceCapabilities.currentExtent;

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.surface = m_vkSurface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageFormat = supportedSurfaceFormats[0].format;
	swapchainCreateInfo.imageExtent.width = m_vkSwapChainExtent.width;
	swapchainCreateInfo.imageExtent.height = m_vkSwapChainExtent.height;
	swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = nullptr;

	uint32_t queueFamilyIndices[2] =
	{
		(uint32_t)m_vkGraphicsQueueFamilyIndex,
		(uint32_t)m_vkPresentQueueFamilyIndex
	};

	if (m_vkGraphicsQueueFamilyIndex != m_vkPresentQueueFamilyIndex)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	m_vkSwapChainFormat = supportedSurfaceFormats[0].format;

	VkResult res = vkCreateSwapchainKHR(m_vkDevice, &swapchainCreateInfo, nullptr, &m_vkSwapChain);

	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &m_vkSwapChainImageCount, nullptr);
	VkImage *swapChainImages = static_cast<VkImage *>(alloca(sizeof(VkImage) * m_vkSwapChainImageCount));
	if (vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &m_vkSwapChainImageCount, swapChainImages) != VK_SUCCESS)
	{
		print("failed to acquire swap chain images\n");
		exit(EXIT_FAILURE);
	}

	if (m_swapChainRenderTargets == nullptr && scope != nullptr)
	{
		m_swapChainRenderTargets = static_cast<RenderTarget **>(scope->allocate(sizeof(RenderTarget) * m_vkSwapChainImageCount));
		for (uint32_t i = 0; i < m_vkSwapChainImageCount; ++i)
			m_swapChainRenderTargets[i] = scope->newObject<RenderTarget>(*this, swapChainImages[i], m_vkSwapChainFormat, VK_SAMPLE_COUNT_1_BIT);
		m_vkSwapChainFramebuffers = static_cast<VkFramebuffer *>(scope->allocate(sizeof(VkFramebuffer) * m_vkSwapChainImageCount));
		m_vkCommandBuffers = static_cast<VkCommandBuffer *>(scope->allocate(sizeof(VkCommandBuffer) * m_vkSwapChainImageCount));
	}
	else
	{
		for (uint32_t i = 0; i < m_vkSwapChainImageCount; ++i)
			m_swapChainRenderTargets[i]->recreate(swapChainImages[i]);
	}
}

void RenderDevice::recreateSwapChain(ScopeStack& scope)
{
	vkDeviceWaitIdle(m_vkDevice);

	cleanupSwapChain();

	createSwapChain();
//	m_depthTarget->resize(m_vkSwapChainExtent.width, m_vkSwapChainExtent.height);
	m_aaRenderTarget->resize(m_vkSwapChainExtent.width, m_vkSwapChainExtent.height);
	m_aaDepthRenderTarget->resize(m_vkSwapChainExtent.width, m_vkSwapChainExtent.height);
	createRenderPass();
	createGraphicsPipeline(scope);
	createFramebuffers();
	createCommandBuffers(*m_scene);
}

void RenderDevice::createRenderPass()
{
	VkAttachmentDescription attachments[3];
	memset(attachments, 0, sizeof(attachments));

	VkAttachmentDescription& aaColorAttachmentDescription = attachments[0];
	aaColorAttachmentDescription.format = m_aaRenderTarget->m_vkFormat;
	aaColorAttachmentDescription.samples = m_aaRenderTarget->m_vkSamples;
	aaColorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	aaColorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	aaColorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	aaColorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	aaColorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	aaColorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference aaColorAttachmentReference = {};
	aaColorAttachmentReference.attachment = 0;
	aaColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription& aaDepthAttachmentDescription = attachments[1];
	aaDepthAttachmentDescription.format = m_aaDepthRenderTarget->m_vkFormat;
	aaDepthAttachmentDescription.samples = m_aaDepthRenderTarget->m_vkSamples;
	aaDepthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	aaDepthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	aaDepthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	aaDepthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	aaDepthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	aaDepthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference aaDepthAttachmentReference = {};
	aaDepthAttachmentReference.attachment = 1;
	aaDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription& resolveAttachmentDescription = attachments[2];
	resolveAttachmentDescription.format = m_vkSwapChainFormat;
	resolveAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	resolveAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	resolveAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	resolveAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	resolveAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference resolveAttachmentReference = {};
	resolveAttachmentReference.attachment = 2;
	resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDescription = {};
	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPassDescription.colorAttachmentCount = 1;
	subPassDescription.pColorAttachments = &aaColorAttachmentReference;
	subPassDescription.pDepthStencilAttachment = &aaDepthAttachmentReference;
	subPassDescription.pResolveAttachments = &resolveAttachmentReference;

	VkSubpassDependency dependencies[2];
	memset(dependencies, 0, sizeof(dependencies));

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = sizeof(attachments) / sizeof(VkAttachmentDescription);
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subPassDescription;
	createInfo.dependencyCount = 2;
	createInfo.pDependencies = dependencies;

	if (vkCreateRenderPass(m_vkDevice, &createInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS)
	{
		print("failed to create render pass\n");
		exit(1);
	}
	else
	{
		print("created render pass\n");
	}
}

void RenderDevice::createFramebuffers()
{
	for (uint32_t i = 0; i < m_vkSwapChainImageCount; i++)
	{
		VkImageView attachements[] = { m_aaRenderTarget->m_vkImageView, m_aaDepthRenderTarget->m_vkImageView, m_swapChainRenderTargets[i]->m_vkImageView };

		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_vkRenderPass;
		createInfo.attachmentCount = sizeof(attachements) / sizeof(VkImageView);
		createInfo.pAttachments = attachements;
		createInfo.width = m_vkSwapChainExtent.width;
		createInfo.height = m_vkSwapChainExtent.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(m_vkDevice, &createInfo, nullptr, &m_vkSwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			print("failed to create framebuffer for swap chain image view #%zd\n", i);
			exit(1);
		}
	}
	print("created framebuffers for swap chain image views.\n");
}

void RenderDevice::createGraphicsPipeline(ScopeStack& scope)
{
	VkShaderModule vertexShaderModule = createShaderModule(scope, "blinnphong.vert.spv");
	VkShaderModule fragmentShaderModule = createShaderModule(scope, "blinnphong.frag.spv");

	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = vertexShaderModule;
	vertexShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &m_vkVertexBindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 3;
	vertexInputCreateInfo.pVertexAttributeDescriptions = m_vkVertexAttributeDescriptions;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_vkSwapChainExtent.width;
	viewport.height = (float)m_vkSwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = m_vkSwapChainExtent.width;
	scissor.extent.height = m_vkSwapChainExtent.height;

	VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.pViewports = &viewport;
	viewportCreateInfo.scissorCount = 1;
	viewportCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
	rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.rasterizationSamples = m_aaRenderTarget->m_vkSamples;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.minSampleShading = 1.0f;
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
	colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	colorBlendCreateInfo.blendConstants[3] = 0.0f;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding ubo2LayoutBinding = {};
	ubo2LayoutBinding.binding = 1;
	ubo2LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo2LayoutBinding.descriptorCount = 1;
	ubo2LayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
/*
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 2;
	samplerLayoutBinding.descriptorCount = 2;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding texturesLayoutBinding = {};
	texturesLayoutBinding.binding = 3;
	texturesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	texturesLayoutBinding.descriptorCount = 3;
	texturesLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	texturesLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding bindings[] = { uboLayoutBinding, ubo2LayoutBinding, samplerLayoutBinding };	// , texturesLayoutBinding
*/
	VkDescriptorSetLayoutBinding textureLayoutBinding = {};
	textureLayoutBinding.binding = 2;
	textureLayoutBinding.descriptorCount = 3;
	textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayoutBinding.pImmutableSamplers = nullptr;
	textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding bindings[] = { uboLayoutBinding, ubo2LayoutBinding, textureLayoutBinding };

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
	descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
	descriptorLayoutCreateInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(m_vkDevice, &descriptorLayoutCreateInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS)
	{
		print("failed to create descriptor layout\n");
		exit(1);
	}
	else
	{
		print("created descriptor layout\n");
	}

	const VkPushConstantRange ranges[] =
	{
		{
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,	// stageFlags
			0,															// offset
			4															// size
		}
	};

	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &m_vkDescriptorSetLayout;
	layoutCreateInfo.pushConstantRangeCount = 1;
	layoutCreateInfo.pPushConstantRanges = ranges;

	if (vkCreatePipelineLayout(m_vkDevice, &layoutCreateInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
	{
		print("failed to create pipeline layout\n");
		exit(1);
	}
	else
	{
		print("created pipeline layout\n");
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.layout = m_vkPipelineLayout;
	pipelineCreateInfo.renderPass = m_vkRenderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS)
	{
		print("failed to create graphics pipeline\n");
		exit(1);
	}
	else
	{
		print("created graphics pipeline\n");
	}

	vkDestroyShaderModule(m_vkDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(m_vkDevice, fragmentShaderModule, nullptr);
}

void RenderDevice::createDescriptorSet(Scene& scene)
{
	VkDescriptorPoolSize poolSizes[3] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = 3;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = 3;
	createInfo.pPoolSizes = poolSizes;
	createInfo.maxSets = 1;

	if (vkCreateDescriptorPool(m_vkDevice, &createInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
	{
		print("failed to create descriptor pool\n");
		exit(1);
	}
	else
	{
		print("created descriptor pool\n");
	}

	// There needs to be one descriptor set per binding point in the shader
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_vkDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_vkDescriptorSetLayout;

	if (vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &m_vkDescriptorSet) != VK_SUCCESS)
	{
		print("failed to create descriptor set\n");
		exit(1);
	}
	else
	{
		print("created descriptor set\n");
	}

	// Update descriptor set with uniform binding
	VkDescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = scene.m_sceneUniformBuffer->m_buffer;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = sizeof(SceneUniformData);

	VkWriteDescriptorSet writeDescriptorSet[3] = {};
	writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[0].dstSet = m_vkDescriptorSet;
	writeDescriptorSet[0].dstBinding = 0;
	writeDescriptorSet[0].dstArrayElement = 0;
	writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet[0].descriptorCount = 1;
	writeDescriptorSet[0].pBufferInfo = &descriptorBufferInfo;

	VkDescriptorBufferInfo descriptorBufferInfo2 = {};
	descriptorBufferInfo2.buffer = scene.m_modelMatrixUniformBuffer->m_buffer;
	descriptorBufferInfo2.offset = 0;
	descriptorBufferInfo2.range = sizeof(ModelUniformData);

	writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[1].dstSet = m_vkDescriptorSet;
	writeDescriptorSet[1].dstBinding = 1;
	writeDescriptorSet[1].dstArrayElement = 0;
	writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet[1].descriptorCount = 1;
	writeDescriptorSet[1].pBufferInfo = &descriptorBufferInfo2;

//	VkDescriptorImageInfo *imageInfo = static_cast<VkDescriptorImageInfo *>(alloca(sizeof(VkDescriptorImageInfo) * m_numTextures));
//	memset(imageInfo, 0, sizeof(VkDescriptorImageInfo) * m_numTextures);

	VkDescriptorImageInfo *texImageInfo = static_cast<VkDescriptorImageInfo *>(alloca(sizeof(VkDescriptorImageInfo) * (m_numTextures+1)));
	memset(texImageInfo, 0, sizeof(VkDescriptorImageInfo) * m_numTextures+1);

	for (uint32_t i = 0; i < m_numTextures; i++)
	{
//		imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//		imageInfo[i].imageView = VK_NULL_HANDLE;
//		imageInfo[i].sampler = m_textures[i]->m_vkSampler;
//		texImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//		texImageInfo[i].imageView = m_textures[i]->m_vkImageView;
//		texImageInfo[i].sampler = VK_NULL_HANDLE;
		texImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texImageInfo[i].imageView = m_textures[i]->m_vkImageView;
		texImageInfo[i].sampler = m_textures[i]->m_vkSampler;
	}
//	texImageInfo[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//	texImageInfo[2].imageView = m_dummyTexture->m_vkImageView;
//	texImageInfo[2].sampler = m_dummyTexture->m_vkSampler;
/*
	for (uint32_t i = 0; i < m_numTextures; i++)
	{
		uint32_t di = 2 + i;
		writeDescriptorSet[di].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet[di].dstSet = m_vkDescriptorSet;
		writeDescriptorSet[di].dstBinding = 2;
		writeDescriptorSet[di].dstArrayElement = 0;
		writeDescriptorSet[di].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writeDescriptorSet[di].descriptorCount = 2;
		writeDescriptorSet[di].pImageInfo = texImageInfo;

		di = 2 + m_numTextures + i;
		writeDescriptorSet[di].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet[di].dstSet = m_vkDescriptorSet;
		writeDescriptorSet[di].dstBinding = 3;
		writeDescriptorSet[di].dstArrayElement = i;
		writeDescriptorSet[di].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writeDescriptorSet[di].descriptorCount = 1;
		writeDescriptorSet[di].pImageInfo = &texImageInfo[i];
	}
*/
	uint32_t di = 2;
	writeDescriptorSet[di].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[di].dstSet = m_vkDescriptorSet;
	writeDescriptorSet[di].dstBinding = 2;
	writeDescriptorSet[di].dstArrayElement = 0;
	writeDescriptorSet[di].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet[di].descriptorCount = 3;
	writeDescriptorSet[di].pImageInfo = texImageInfo;

	vkUpdateDescriptorSets(m_vkDevice, 2+1, writeDescriptorSet, 0, nullptr);
}

void RenderDevice::createCommandBuffers(Scene& scene)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = m_vkCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = m_vkSwapChainImageCount;

	if (vkAllocateCommandBuffers(m_vkDevice, &commandBufferAllocateInfo, m_vkCommandBuffers) != VK_SUCCESS)
	{
		print("failed to create command buffers\n");
		exit(EXIT_FAILURE);
	}

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	// Note: contains value for each subresource range
	VkClearValue clearValues[2];
	clearValues[0].color = { 0.0f, 0.0f, 0.25f, 1.0f };  // R, G, B, A
	clearValues[1].depthStencil = { 1.0f, 0 };			 // Depth / Stencil

	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;

	// Record the command buffer for every swap chain image
	for (uint32_t i = 0; i < m_vkSwapChainImageCount; i++) 
	{
		// Record command buffer
		vkBeginCommandBuffer(m_vkCommandBuffers[i], &commandBufferBeginInfo);

		// Change layout of image to be optimal for clearing
		// Note: previous layout doesn't matter, which will likely cause contents to be discarded
		VkImageMemoryBarrier presentToDrawBarrier = {};
		presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		presentToDrawBarrier.srcAccessMask = 0;
		presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		if (m_vkPresentQueueFamilyIndex == m_vkGraphicsQueueFamilyIndex)
		{
			presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		}
		else
		{
			presentToDrawBarrier.srcQueueFamilyIndex = m_vkPresentQueueFamilyIndex;
			presentToDrawBarrier.dstQueueFamilyIndex = m_vkGraphicsQueueFamilyIndex;
		}
		presentToDrawBarrier.image = m_swapChainRenderTargets[i]->m_vkImage;
		presentToDrawBarrier.subresourceRange = subResourceRange;

		vkCmdPipelineBarrier(m_vkCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentToDrawBarrier);

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = m_vkRenderPass;
		renderPassBeginInfo.framebuffer = m_vkSwapChainFramebuffers[i];
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent = m_vkSwapChainExtent;
		renderPassBeginInfo.clearValueCount = sizeof(clearValues) / sizeof(VkClearValue);
		renderPassBeginInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(m_vkCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout, 0, 1, &m_vkDescriptorSet, 0, nullptr);

		vkCmdBindPipeline(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(m_vkCommandBuffers[i], 0, 1, &m_vertexBuffer->m_buffer, &offset);

		vkCmdBindIndexBuffer(m_vkCommandBuffers[i], m_indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);

		for (uint32_t draw = 0; draw < scene.m_meshInstanceCount; ++draw)
		{
			if (scene.m_instanceMeshes[draw] != nullptr)
			{
				vkCmdPushConstants(m_vkCommandBuffers[i], m_vkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4, &draw);
				vkCmdDrawIndexed(m_vkCommandBuffers[i], scene.m_instanceMeshes[draw]->m_renderables[0].getIndexCount(), 1, scene.m_instanceMeshes[draw]->m_renderables[0].getFirstIndex(), scene.m_instanceMeshes[draw]->m_renderables[0].getFirstVertex(), 0);
			}
		}

		vkCmdEndRenderPass(m_vkCommandBuffers[i]);

		// If present and graphics queue families differ, then another barrier is required
		if (m_vkPresentQueueFamilyIndex != m_vkGraphicsQueueFamilyIndex)
		{
			VkImageMemoryBarrier drawToPresentBarrier = {};
			drawToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			drawToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			drawToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			drawToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			drawToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			drawToPresentBarrier.srcQueueFamilyIndex = m_vkGraphicsQueueFamilyIndex;
			drawToPresentBarrier.dstQueueFamilyIndex = m_vkPresentQueueFamilyIndex;
			drawToPresentBarrier.image = m_swapChainRenderTargets[i]->m_vkImage;
			drawToPresentBarrier.subresourceRange = subResourceRange;

			vkCmdPipelineBarrier(m_vkCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &drawToPresentBarrier);
		}

		if (vkEndCommandBuffer(m_vkCommandBuffers[i]) != VK_SUCCESS) 
		{
			print("failed to record command buffer\n");
			exit(EXIT_FAILURE);
		}
	}

	print("recorded command buffers\n");

	vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
}

int32_t RenderDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties& deviceMemoryProperties = m_vkPhysicalDeviceMemoryProperties[m_selectedDevice];

	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & (1 << i)) && ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
		{
//			print("typeBits = 0x%08x -> typeIndex = %d\n", typeBits, i);
			return i;
		}
	}
	print("Could not find memory type to satisfy requirements\n");
	return -1;
}

VkDeviceMemory RenderDevice::allocateGpuMemory(VkDeviceSize size, VkDeviceSize alignment, uint32_t typeIndex)
{
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = typeIndex;
	VkDeviceMemory memory;
	VkResult res = vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &memory);	// &m_vkDepthBufferMemory);
	AssertMsg(res == VK_SUCCESS, "failed to allocate memory for image\n");

	return memory;
}

VkShaderModule RenderDevice::createShaderModule(ScopeStack& scope,  const char *filename)
{
	File file(filename, "Shaders/");
	file.load(scope);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = file.m_sizeInBytes;
	createInfo.pCode = reinterpret_cast<uint32_t *>(file.m_buffer);

#if 0
	{
		SpvReflectShaderModule module;
		SpvReflectResult result = spvReflectCreateShaderModule(createInfo.codeSize, createInfo.pCode, &module);
		AssertMsg((result == SPV_REFLECT_RESULT_SUCCESS), "Failed to create SPIRV-Reflect Module.\n");

		// Enumerate and extract shader's input variables
/*
		uint32_t inputVariableCount = 0;
		result = spvReflectEnumerateInputVariables(&module, &inputVariableCount, NULL);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);
		SpvReflectInterfaceVariable* inputVariables = (SpvReflectInterfaceVariable*)scope.allocate(inputVariableCount * sizeof(SpvReflectInterfaceVariable));
		result = spvReflectEnumerateInputVariables(&module, &inputVariableCount, inputVariables);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		uint32_t outputVariableCount = 0;
		result = spvReflectEnumerateOutputVariables(&module, &outputVariableCount, NULL);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);
		SpvReflectInterfaceVariable* outputVariables = (SpvReflectInterfaceVariable*)scope.allocate(outputVariableCount * sizeof(SpvReflectInterfaceVariable));
		result = spvReflectEnumerateOutputVariables(&module, &outputVariableCount, outputVariables);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);
*/
		uint32_t descriptorBindingCount = 0;
		spvReflectEnumerateDescriptorSets(&module, &descriptorBindingCount, NULL);
		SpvReflectDescriptorSet* descriptorBindings = (SpvReflectDescriptorSet*)scope.allocate(descriptorBindingCount * sizeof(SpvReflectDescriptorSet));
		result = spvReflectEnumerateDescriptorSets(&module, &descriptorBindingCount, &descriptorBindings);

		// Output variables, descriptor bindings, descriptor sets, and push constants
		// can be enumerated and extracted using a similar mechanism.

		// Destroy the reflection data when no longer required.
		spvReflectDestroyShaderModule(&module);
	}
#endif

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_vkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		print("failed to create shader module for %s\n", filename);
		exit(1);
	}

	print("created shader module for %s\n", filename);

	return shaderModule;
}

VkCommandBuffer RenderDevice::beginSingleUseCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_vkCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void RenderDevice::endSingleUseCommandBuffer(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkFence fence;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (vkCreateFence(m_vkDevice, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS)
	{
		print("failed to create fence.\n");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS)
	{
		print("failed to submit to queue.\n");
	}

	if (vkWaitForFences(m_vkDevice, 1, &fence, VK_TRUE, kDefaultFenceTimeout) != VK_SUCCESS)
	{
		print("failed to wait on fence.\n");
	}

	vkDestroyFence(m_vkDevice, fence, nullptr);

	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &commandBuffer);
}

void RenderDevice::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageSubresourceRange& subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange = subresourceRange;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED || oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else
	{
		print("unsupported layout transition!\n");
		exit(1);
	}
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void RenderDevice::copyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height)
{
	VkImageSubresourceLayers subResource = {};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource.baseArrayLayer = 0;
	subResource.mipLevel = 0;
	subResource.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = subResource;
	region.dstSubresource = subResource;
	region.srcOffset = { 0, 0, 0 };
	region.dstOffset = { 0, 0, 0 };
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

RenderDevice::GPUMemoryManager::GPUMemoryManager(RenderDevice& renderDevice)
	: m_renderDevice(renderDevice)
	, m_numBlocks(0)
	, m_bufferImageGranularity(0)
{
	for (uint32_t i = 0; i < kMaxGPUMemoryBlocks; ++i)
		m_blocks[i] = nullptr;
}

RenderDevice::GPUMemoryManager& RenderDevice::GPUMemoryManager::Instance(RenderDevice *renderDevice)
{
	static GPUMemoryManager ms_instance(*renderDevice);
	FatalAssertMsg((&ms_instance.m_renderDevice != nullptr), "Attempted to initialize GPUMemoryManager without a RenderDevice.\n");
	return ms_instance;
}

GPUMemAllocInfo& RenderDevice::GPUMemoryManager::allocate(ScopeStack& scope, VkDeviceSize size, VkDeviceSize alignment, uint32_t typeIndex)
{
	AssertMsg((size < kGPUMemoryBlockSize), "Requested allocation larger than kGPUMemoryBlockSize.\n");
	uint32_t size32 = static_cast<uint32_t>(size);
	uint32_t alignment32 = static_cast<uint32_t>(alignment);
	uint32_t optimal = typeIndex >> 8;	// optimal will be 1 if we're using OPTIMAL tiling mode, 0 if LINEAR.
	typeIndex = typeIndex & 0xff;
	GPUMemoryBlock& memBlock = findBlock(scope, size32, alignment32, typeIndex);
	print("GPU allocate size = %ld, alignment = %ld, typeIndex = %d\n", size, alignment, typeIndex);
	alignment32 = (optimal != memBlock.m_optimal) ? std::max(alignment32, m_bufferImageGranularity) : alignment32;
	return memBlock.allocate(scope, size32, alignment32, optimal);
}

static uint32_t align(uint32_t size, uint32_t align = 16)
{
	return (size + (align - 1)) & ~(align - 1);
}

GPUMemoryBlock& RenderDevice::GPUMemoryManager::findBlock(ScopeStack& scope, VkDeviceSize size, VkDeviceSize alignment, uint32_t typeIndex)
{
	for (uint32_t i = 0; i < m_numBlocks; ++i)
	{
		if (m_blocks[i] != nullptr && m_blocks[i]->m_typeIndex == typeIndex)
		{
			uint32_t alignedOffset = align(m_blocks[i]->m_offset, static_cast<uint32_t>(alignment));
			if ((m_blocks[i]->m_size - alignedOffset) >= size)
				return *m_blocks[i];
		}
	}
	FatalAssertMsg((m_numBlocks < kMaxGPUMemoryBlocks), "Out of memory!\n");
	GPUMemoryBlock *newBlock = m_blocks[m_numBlocks++] = static_cast<GPUMemoryBlock *>(scope.newObject<GPUMemoryBlock>(m_renderDevice.m_vkDevice, kGPUMemoryBlockSize, typeIndex));

	return *newBlock;
}

GPUMemoryBlock::GPUMemoryBlock(VkDevice vkDevice, uint32_t size, uint32_t typeIndex)
	: m_vkDevice(vkDevice)
	, m_memory(nullptr)
	, m_size(0)
	, m_offset(0)
	, m_typeIndex(typeIndex)
	, m_optimal(0)
{
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = typeIndex;

	VkResult res = vkAllocateMemory(vkDevice, &allocInfo, nullptr, &m_memory);
	if (res == VK_SUCCESS)
		m_size = size;
}

GPUMemoryBlock::~GPUMemoryBlock()
{
	if (m_vkDevice && m_memory)
		vkFreeMemory(m_vkDevice, m_memory, nullptr);
}

GPUMemAllocInfo& GPUMemoryBlock::allocate(ScopeStack& scope, uint32_t size, uint32_t alignment, uint32_t optimal)
{
	uint32_t offset = align(m_offset, alignment);
	m_offset = offset + size;
	m_optimal = optimal;
	GPUMemAllocInfo *info = scope.newObject<GPUMemAllocInfo>(*this, size, offset);

	return *info;
}

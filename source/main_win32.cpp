#include "stdafx.h"
#include "Vulkan/RenderDeviceVk.h"

const uint32_t kMemMgrSize = 256 * 1024 * 1024;
const uint32_t kMemMgrAlign = 1024 * 1024;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

static void size_callback(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

	RenderDevice *pRenderDevice = static_cast<RenderDevice *>(glfwGetWindowUserPointer(window));
	if (pRenderDevice)
	{
		Application::GetApplication()->resize(*pRenderDevice, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	}
}

int main(int argc, char *argv[])
{
	Application* app = Application::GetApplication();

	if (!glfwInit())
	{
		print("glfwInit failed.\n");
		exit(EXIT_FAILURE);
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(app->getScreenWidth(), app->getScreenHeight(), app->getApplicationName(), nullptr /*glfwGetPrimaryMonitor()*/, nullptr);

	uint8_t *memoryBlock = static_cast<uint8_t *>(_aligned_malloc(kMemMgrSize, kMemMgrAlign));
	LinearAllocator allocator(memoryBlock, kMemMgrSize);
	{
		ScopeStack systemScope(allocator, "System");

		const GLFWvidmode *primaryScreenMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		RenderDevice *pRenderDevice = systemScope.newObject<RenderDevice>(systemScope, static_cast<uint32_t>(primaryScreenMode->width), static_cast<uint32_t>(primaryScreenMode->height), window);
#if defined(WIN32)
		app->setGLFWwindow(window);
#endif
		ScopeStack initScope(systemScope, "AppInit");

		app->initialize(initScope, *pRenderDevice);

		app->resize(*pRenderDevice, app->getScreenWidth(), app->getScreenHeight());

		glfwSetWindowUserPointer(window, pRenderDevice);
		glfwSetKeyCallback(window, key_callback);
		glfwSetWindowSizeCallback(window, size_callback);

		uint64_t frameNum = 0;
		while (!glfwWindowShouldClose(window))
		{
			char frameName[16];
			sprintf_s(frameName, sizeof(frameName), "Frame %lld", frameNum++);
			ScopeStack frameScope(initScope, frameName, false);
			app->update(frameScope, *pRenderDevice);
			app->render(frameScope, *pRenderDevice);

			glfwPollEvents();
		}
	}
	glfwDestroyWindow(window);
}


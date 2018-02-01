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

	Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
	if (app)
	{
		RenderDevice& rd = app->getRenderDevice();
		rd.recreateSwapChain();
		app->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	}
}

int main(int argc, char *argv[])
{
	uint8_t *memoryBlock = static_cast<uint8_t *>(_aligned_malloc(kMemMgrSize, kMemMgrAlign));
	LinearAllocator allocator(memoryBlock, kMemMgrSize);
	{
		ScopeStack scopeStack(allocator, "Main");

		Application* app = Application::GetApplication();

		if (!glfwInit())
		{
			print("glfwInit failed.\n");
			exit(EXIT_FAILURE);
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		GLFWwindow* window = glfwCreateWindow(app->getScreenWidth(), app->getScreenHeight(), app->getApplicationName(), nullptr, nullptr);
#if defined(WIN32)
		app->setGLFWwindow(window);
#endif
		app->initialize(scopeStack);

		glfwSetWindowUserPointer(window, app);
		glfwSetKeyCallback(window, key_callback);
		glfwSetWindowSizeCallback(window, size_callback);

		while (!glfwWindowShouldClose(window))
		{
			app->update();
			app->render();

			glfwPollEvents();
		}

		app->cleanup();
		glfwDestroyWindow(window);
	}
}


#include "stdafx.h"
#include "Vulkan/RenderDeviceVk.h"

const uint32_t kMemMgrSize = 128 * 1024 * 1024;
const uint32_t kMemMgrAlign = 1024 * 1024;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

static void size_callback(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

	RenderDevice *renderDevice = static_cast<RenderDevice *>(glfwGetWindowUserPointer(window));
		renderDevice->recreateSwapChain();
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

	GLFWwindow* window = glfwCreateWindow(app->getScreenWidth(), app->getScreenHeight(), app->getApplicationName(), nullptr, nullptr);
	app->setGLFWwindow(window);
	app->initialize();

	glfwSetWindowUserPointer(window, &app->getRenderDevice());
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


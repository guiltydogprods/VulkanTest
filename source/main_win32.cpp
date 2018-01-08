#include "stdafx.h"

const uint32_t kMemMgrSize = 128 * 1024 * 1024;
const uint32_t kMemMgrAlign = 1024 * 1024;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(app->getScreenWidth(), app->getScreenHeight(), app->getApplicationName(), nullptr, nullptr);
	app->setGLFWwindow(window);
	app->initialize();

	glfwSetKeyCallback(window, key_callback);

	while (!glfwWindowShouldClose(window))
	{
		app->update();
		app->render();

		glfwPollEvents();
	}

	app->cleanup();

	glfwDestroyWindow(window);
}


#pragma once

class Application
{
public:
	Application(const char *appName, uint32_t screenWidth, uint32_t screenHeight);
	virtual ~Application();

	static Application* GetApplication() {	return ms_application;	}

	virtual void initialize();
	virtual void cleanup();
	virtual void update();
	virtual void render();

	inline const char *getApplicationName()				{ return m_appName;				}
	inline uint32_t getScreenWidth()					{ return m_screenWidth;			}
	inline uint32_t getScreenHeight()					{ return m_screenHeight;		}
	inline GLFWwindow *getGLFWwindow()					{ return m_glfwWindow;			}
	inline void setGLFWwindow(GLFWwindow *glfwWindow)	{ m_glfwWindow = glfwWindow;	}

protected:
	const char			*m_appName;
	uint32_t			m_screenWidth;
	uint32_t			m_screenHeight;
	GLFWwindow			*m_glfwWindow;
	static Application *ms_application;
};

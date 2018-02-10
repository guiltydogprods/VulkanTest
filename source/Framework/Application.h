#pragma once

struct RenderDevice;

class Application
{
public:
	Application(const char *appName, uint32_t screenWidth, uint32_t screenHeight);
	virtual ~Application();

	static Application* GetApplication() {	return ms_application;	}

	virtual void initialize(ScopeStack& scopeStack, RenderDevice& renderDevice);
	virtual void cleanup();
	virtual void update(ScopeStack& frameScope, RenderDevice& renderDevice);
	virtual void render(ScopeStack& frameScope, RenderDevice& renderDevice);
	virtual void resize(uint32_t width, uint32_t height);

	inline const char *getApplicationName()				{ return m_appName;				}
	inline uint32_t getScreenWidth()					{ return m_screenWidth;			}
	inline uint32_t getScreenHeight()					{ return m_screenHeight;		}
#if defined(WIN32)
	inline GLFWwindow *getGLFWwindow()					{ return m_glfwWindow;			}
	inline void setGLFWwindow(GLFWwindow *glfwWindow)	{ m_glfwWindow = glfwWindow;	}
#endif // defined(WIND32)
	inline bool getWasResize()							{ return m_bWasResized;			}
	inline void clearWasResize()						{ m_bWasResized = false;		}

protected:
	const char			*m_appName;
	uint32_t			m_screenWidth;
	uint32_t			m_screenHeight;
#if defined(WIN32)
	GLFWwindow			*m_glfwWindow;
#endif // defined(WIND32)
	bool				*m_bWasResized;
	static Application *ms_application;
};

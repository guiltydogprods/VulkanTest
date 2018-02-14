#include "stdafx.h"

Application *Application::ms_application = nullptr;

Application::Application(const char *applicationName)
	: m_appName(applicationName)
	, m_bWasResized(false)
{
	ms_application = this;
}

Application::~Application()
{
	ms_application = nullptr;
}

void Application::initialize(ScopeStack& scopeStack, RenderDevice& renderDevice)
{
}

void Application::cleanup()
{

}

void Application::update(ScopeStack& frameScope, RenderDevice& renderDevice)
{

}

void Application::render(ScopeStack& frameScope, RenderDevice& renderDevice)
{

}

void Application::resize(uint32_t width, uint32_t height)
{
	m_screenWidth = width;
	m_screenHeight = height;
	m_bWasResized = true;
}

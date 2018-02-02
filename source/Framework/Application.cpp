#include "stdafx.h"

Application *Application::ms_application = nullptr;

Application::Application(const char *applicationName, uint32_t screenWidth, uint32_t screenHeight)
	: m_appName(applicationName)
	, m_screenWidth(screenWidth)
	, m_screenHeight(screenHeight)
	, m_pRenderDevice(nullptr)
{
	ms_application = this;
}

Application::~Application()
{
	ms_application = nullptr;
}

void Application::initialize(ScopeStack& scopeStack)
{
}

void Application::cleanup()
{

}

void Application::update(ScopeStack& frameScope)
{

}

void Application::render(ScopeStack& frameScope)
{

}

void Application::resize(uint32_t width, uint32_t height)
{
}

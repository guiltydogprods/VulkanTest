#include "stdafx.h"

Application *Application::ms_application = nullptr;

Application::Application(const char *applicationName, uint32_t screenWidth, uint32_t screenHeight)
	: m_appName(applicationName)
	, m_screenWidth(screenWidth)
	, m_screenHeight(screenHeight)
{
	ms_application = this;
}

Application::~Application()
{
	ms_application = nullptr;
}

void Application::initialize()
{
}

void Application::cleanup()
{

}

void Application::update()
{

}

void Application::render()
{

}
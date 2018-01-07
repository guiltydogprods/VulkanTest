#include "stdafx.h"
#include "ThunderBall.h"
#include "vulkan/renderdevice.h"

ThunderBallApp s_thundeBallApp;

const char		*kApplicationName = "Thunder Ball";
const uint32_t	kScreenWidth = 1920;
const uint32_t	kScreenHeight = 1080;

ThunderBallApp::ThunderBallApp()
	: Application(kApplicationName, kScreenWidth, kScreenHeight)
	, m_pRenderDevice(nullptr)
{
}

ThunderBallApp::~ThunderBallApp()
{

}

void ThunderBallApp::initialize()
{
	m_pRenderDevice = new RenderDevice();
}


void ThunderBallApp::update()
{
	m_pRenderDevice->update();
}

void ThunderBallApp::render()
{
	m_pRenderDevice->render();
}

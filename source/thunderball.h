#pragma once 

class RenderDevice;

class ThunderBallApp : public Application
{
public:
	ThunderBallApp();
	~ThunderBallApp();

	void initialize();
	void update();
	void render();

protected:
	RenderDevice *m_pRenderDevice;
};

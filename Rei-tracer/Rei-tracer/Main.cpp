#include "Core.h"

int main(int argc, char** argv)
{

	Core::CreateInstance();
	Core* core = Core::GetInstance();
	core->Init(400, 400, false);
	InputManager* input = core->GetInputManager();
	IGraphics* graphics = core->GetGraphics();

	graphics->AddSphere(-5.0f, 2.0f, -20.0f, 3.0f);
	graphics->AddSphere(10.0f, -4.0f, -10.0f, 3.0f);
	graphics->AddSphere(15.0f, -8.0f, -40.0f, 3.0f);
	graphics->AddSphere(0.0f, -1.0f, -40.0f, 3.0f);
	graphics->AddPlane(1.0f, 0.0f, 0.0f, -4.0f);
	graphics->AddPlane(-1.0f, 0.0f, 0.0f, 4.0f);
	graphics->AddPlane(0.0f, 1.0f, 0.0f, -4.0f);
	graphics->AddPlane(0.0f, -1.0f, 0.0f, 4.0f);
	graphics->AddPlane(0.0f, 0.0f, 1.0f, -50.0f);

	while (!input->IsKeyDown(SDLK_ESCAPE))
	{
		core->Update();
	}

	Core::ShutDown();
	return 0;
}
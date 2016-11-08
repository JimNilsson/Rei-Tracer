#include "Core.h"

int main(int argc, char** argv)
{

	Core::CreateInstance();
	Core* core = Core::GetInstance();
	core->Init(800, 640, false);
	InputManager* input = core->GetInputManager();
	IGraphics* graphics = core->GetGraphics();


	while (!input->IsKeyDown(SDLK_ESCAPE))
	{
		core->Update();
	}

	Core::ShutDown();
	return 0;
}
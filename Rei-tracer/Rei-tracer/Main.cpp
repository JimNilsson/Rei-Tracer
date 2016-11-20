#include "Core.h"

int main(int argc, char** argv)
{

	Core::CreateInstance();
	Core* core = Core::GetInstance();
	core->Init(400, 400, false);
	InputManager* input = core->GetInputManager();
	IGraphics* graphics = core->GetGraphics();
	CameraManager* cam = core->GetCameraManager();
	Window* window = core->GetWindow();
	Timer* timer = core->GetTimer();

	cam->AddCamera(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 3.14f / 2.0f, (float)window->GetWidth() / (float)window->GetHeight(), 0.0f, 1.0f, 0.0f, 1.0f, 50.0f);
	cam->CycleActiveCamera();

	Sphere spheres[5];
	spheres[0] = Sphere(-0.0f, 10.0f, -0.0f, 3.0f);
	spheres[1] = Sphere(10.0f, 4.0f, -10.0f, 3.0f);
	spheres[2] = Sphere(15.0f, 8.0f, -40.0f, 3.0f);
	spheres[3] = Sphere(20.0f, 4.0f, -30.0f, 3.0f);
	spheres[4] = Sphere(5.0f, 5.0f, -5.0f, 0.05f);
	graphics->SetSpheres(spheres, 5);


	PointLight pointlights[1];
	pointlights[0] = PointLight(0, -10.0f, -25.0f, 1.0f, 1.0f, 1.0f, 1.0f, 50.0f);
	graphics->SetPointLights(pointlights, 1);


	float dt = 0.0f;
	while (!input->IsKeyDown(SDLK_ESCAPE))
	{
		dt = timer->GetDeltaTime();
		if(input->IsKeyDown(SDLK_RIGHT))
			cam->RotateYaw(dt * 0.05f);
		if (input->IsKeyDown(SDLK_LEFT))
			cam->RotateYaw(dt * -0.05f);
		if (input->IsKeyDown(SDLK_UP))
			cam->RotatePitch(dt * -0.05f);
		if (input->IsKeyDown(SDLK_DOWN))
			cam->RotatePitch(dt * 0.05f);
		if (input->IsKeyDown(SDLK_w))
			cam->MoveForward(dt * 2.5f);
		if (input->IsKeyDown(SDLK_s))
			cam->MoveForward(dt * -2.5f);
		if (input->IsKeyDown(SDLK_a))
			cam->MoveRight(dt * -2.5f);
		if (input->IsKeyDown(SDLK_d))
			cam->MoveRight(dt * 2.5f);
		core->Update();
	}

	Core::ShutDown();
	return 0;
}
#include "Core.h"
#include <sstream>
#include "OBJLoader.h"
int main(int argc, char** argv)
{

	Core::CreateInstance();
	Core* core = Core::GetInstance();
	core->Init(800, 640, false);
	InputManager* input = core->GetInputManager();
	IGraphics* graphics = core->GetGraphics();
	CameraManager* cam = core->GetCameraManager();
	Window* window = core->GetWindow();
	Timer* timer = core->GetTimer();

	cam->AddCamera(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 3.14f / 2.0f, (float)window->GetWidth() / (float)window->GetHeight(), 0.0f, 1.0f, 0.0f, 1.0f, 50.0f);
	cam->CycleActiveCamera();

	Sphere spheres[5];
	spheres[0] = Sphere(4.0f, 10.0f, -5.0f, 1.0f);
	spheres[1] = Sphere(10.0f, 4.0f, -15.0f, 1.0f);
	spheres[2] = Sphere(15.0f, 8.0f, -5.0f, 1.0f);
	spheres[3] = Sphere(17.0f, 4.0f, -10.0f, 1.0f);
	spheres[4] = Sphere(5.0f, 5.0f, -5.0f, 1.0f);
	//graphics->SetSpheres(spheres, 5);

	OBJLoader objLoader;

	Triangle triangles[MAX_TRIANGLES];
	triangles[0] = Triangle(TriangleVertex(0, 0, 0, 0,
		0, 1, 0, 0,
		1, 0, 0, 0),
		TriangleVertex(100, 0, 0, 1,
			0, 1, 0, 0,
			1, 0, 0, 0),
		TriangleVertex(0, 0, -100, 0,
			0, 1, 0, 1,
			1, 0, 0, 0));
	triangles[1] = Triangle(TriangleVertex(0, 20, 0, 0,
		0, -1, 0, 0,
		1, 0, 0, 0),
		TriangleVertex(100, 20, 0, 1,
			0, -1, 0, 0,
			1, 0, 0, 0),
		TriangleVertex(0, 20, -100, 0,
			0, -1, 0, 1,
			1, 0, 0, 0));
	triangles[2] = Triangle(TriangleVertex(0, 0, 0, 0,
		1, 0, 0, 0,
		1, 0, 0, 0),
		TriangleVertex(0, 0, -100, 1,
			1, 0, 0, 0,
			0, 1, 0, 0),
		TriangleVertex(0, 100, 0, 0,
			1, 0, 0, 1,
			0, 1, 0, 0));
	triangles[3] = Triangle(TriangleVertex(20, 0, 0, 0,
		-1, 0, 0, 0,
		1, 0, 0, 0),
		TriangleVertex(20, 0, -100, 1,
			-1, 0, 0, 0,
			0, 1, 0, 0),
		TriangleVertex(20, 100, 0, 0,
			0, -1, 0, 1,
			0, 1, 0, 0));
	triangles[4] = Triangle(
		TriangleVertex(-50, 0, -20, 0,
		0, 0, 1, 0,
		1, 0, 0, 0),
		TriangleVertex(100, 0, -20, 1,
			0, 0, 1, 0,
			0, 1, 0, 0),
		TriangleVertex(100, 100, -20, 0,
			0, 0, 1, 1,
			0, 1, 0, 0));
	triangles[5] = Triangle(
		TriangleVertex(100, 0, 0, 0,
			0, 0, -1, 0,
			-1, 0, 0, 0),
		TriangleVertex(100, 100, 0, 1,
			0, 0, -1, 0,
			-1, 0, 0, 0),
		TriangleVertex(-100, 0, 0, 0,
			0, 0, -1, 1,
			-1, 0, 0, 0));
	unsigned trianglesAdded = objLoader.LoadOBJ("torus.obj", &triangles[6], MAX_TRIANGLES - 6);
	graphics->SetTriangles(triangles, 6 /*+ trianglesAdded*/);

	PointLight pointlights[10];
	pointlights[0] = PointLight(10.0f, 15.0f, -8.0f, 0.63f, 0.0f, 1.0f, 0.7f, 15.0f);
	pointlights[1] = PointLight(10.0f, 12.0f, -5.0f, 0.63f, 0.0f, 0.3f, 1.0f, 15.0f);
	pointlights[2] = PointLight(12.0f, 7.0f, -15.0f, 0.63f, 0.8f, 0.8f, 0.2f, 15.0f);
	pointlights[3] = PointLight(5.0f, 10.0f, -18.0f, 0.63f, 1.0f, 0.0f, 1.0f, 15.0f);
	graphics->SetPointLights(pointlights, 4);
	graphics->SetBounceCount(2);

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
		if (input->WasKeyPressed(SDLK_o))
			graphics->IncreaseBounceCount();
		if (input->WasKeyPressed(SDLK_p))
			graphics->DecreaseBounceCount();
		core->Update();
		std::stringstream ss;
		ss << "FPS: " << static_cast<int>(1.0f / timer->GetDeltaTime());

		Core::GetInstance()->GetWindow()->SetTitle(ss.str());
	}

	Core::ShutDown();
	return 0;
}
#include "Core.h"
#include <sstream>
#include "OBJLoader.h"
#include <crtdbg.h>

int main(int argc, char** argv)
{
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);

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
	spheres[0] = Sphere(-6.0f, 0.0f, -5.0f, 1.0f);
	spheres[1] = Sphere(0.0f, 4.0f, -5.0f, 1.0f);
	spheres[2] = Sphere(5.0f, -8.0f, -2.5f, 1.0f);
	spheres[3] = Sphere(8.0f, -6.0f, -3.0f, 0.3f);
	spheres[4] = Sphere(-5.0f, 5.0f, -5.0f, 1.0f);
	graphics->SetSpheres(spheres, 5);

	OBJLoader objLoader;
#pragma region
	Triangle triangles[MAX_TRIANGLES];
	//Floor
	triangles[0] = Triangle(TriangleVertex(-10, -10, 10, 0,
		0, 1, 0, 0,
		1, 0, 0, 0),
		TriangleVertex(40, -10, 10, 2,
			0, 1, 0, 0,
			1, 0, 0, 1),
		TriangleVertex(-10, -10, -40, 0,
			0, 1, 0, 1,
			1, 0, 0, 0));
	//Roof
	triangles[1] = Triangle(TriangleVertex(-10, 10, 10, 0,
		0, -1, 0, 0,
		1, 0, 0, 0),
		TriangleVertex(-10, 10, -100, 0,
			0, -1, 0, 1,
			1, 0, 0, 0),
		TriangleVertex(100, 10, 10, 1,
			0, -1, 0, 0,
			1, 0, 0, 0));
	//right wall
	triangles[2] = Triangle(TriangleVertex(-10, -10, 10, 0,
		1, 0, 0, 0,
		1, 0, 0, 0),
		TriangleVertex(-10, -10, -100, 1,
			1, 0, 0, 0,
			0, 1, 0, 0),
		TriangleVertex(-10, 100, 10, 0,
			1, 0, 0, 1,
			0, 1, 0, 0));
	//left wall
	triangles[3] = Triangle(TriangleVertex(10, -10, 10, 0,
		-1, 0, 0, 0,
		1, 0, 0, 0),
		TriangleVertex(10, 100, 10, 0,
			-1, 0, 0, 1,
			0, 1, 0, 0),
		TriangleVertex(10, -10, -100, 1,
			-1, 0, 0, 0,
			0, 1, 0, 0)
		);
	//back wall
	triangles[4] = Triangle(
		TriangleVertex(-50, -10, -10, 0,
			0, 0, 1, 0,
			1, 0, 0, 0),
		TriangleVertex(100, -10, -10, 1,
			0, 0, 1, 0,
			0, 1, 0, 0),
		TriangleVertex(100, 100, -10, 0,
			0, 0, 1, 1,
			0, 1, 0, 0));
	//Front wall
	triangles[5] = Triangle(
		TriangleVertex(100, -10, 10, 0,
			0, 0, -1, 0,
			-1, 0, 0, 0),
		TriangleVertex(-100, -10, 10, 0,
			0, 0, -1, 1,
			-1, 0, 0, 0),
		TriangleVertex(100, 100, 10, 1,
			0, 0, -1, 0,
			-1, 0, 0, 0));
#pragma endregion
	unsigned trianglesAdded = objLoader.LoadOBJ("cube.obj", &triangles[6], MAX_TRIANGLES - 6);
	unsigned tadd2 = objLoader.LoadOBJ("sphere1.obj", &triangles[6 + trianglesAdded], MAX_TRIANGLES - 6 - trianglesAdded);
	unsigned nodecount;
	OctNode* tree = nullptr;
	objLoader.PartitionMesh(&triangles[6 + trianglesAdded], tadd2,6 + trianglesAdded, &tree, 2, nodecount, MAX_TRIANGLES - 6 - trianglesAdded - tadd2);
	MeshIndices mi[2];
	mi[0].lowerIndex = 0;
	mi[0].upperIndex = 6 + trianglesAdded;
	mi[0].rootPartition = -1;
	mi[0].partitionCount = -1;
	mi[1].lowerIndex = 6 + trianglesAdded;
	mi[1].upperIndex = 6 + trianglesAdded + tadd2;
	mi[1].rootPartition = 0;
	mi[1].partitionCount = nodecount;

	graphics->SetMeshPartitions(tree, mi, nodecount, 2);
	
	graphics->SetTriangles(triangles, 6 + trianglesAdded + tadd2);
	graphics->PrepareTextures(0, 0, "ft_stone01_c.png", "ft_stone01_n.png");
	graphics->PrepareTextures(6, 6 + trianglesAdded - 1, "ft_stone01_c.png", "ft_stone01_n.png");
	graphics->PrepareTextures(6 + trianglesAdded, 6 + trianglesAdded + tadd2, "lunarrock_s.png", "lunarrock_n.png");
	graphics->SetTextures();



	PointLight pointlights[10];
	pointlights[0] = PointLight(3.710f, 1.333f, -4.172f, 0.63f, 0.0f, 1.0f, 0.7f, 15.0f);
	pointlights[1] = PointLight(5.0f, 0.25f, -5.0f, 0.63f, 0.0f, 0.3f, 1.0f, 15.0f);
	pointlights[2] = PointLight(3.0f, 5.0f, 0.0f, 0.63f, 0.8f, 0.8f, 0.8f, 15.0f);
	pointlights[3] = PointLight(5.0f, 5.0f, -8.0f, 0.63f, 1.0f, 0.0f, 1.0f, 15.0f);
	graphics->SetPointLights(pointlights, 4);

	SpotLight spotlights[10];
	spotlights[0] = SpotLight(-2.0f, 0.0f, 2.0f, 0.70f,
		1.0f, 1.0f, 1.0f, 20.0f,
		0.7071f, 0.0f, -0.7071f, 9.0f);
	graphics->SetSpotLights(spotlights, 1);
	graphics->SetBounceCount(0);

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
	delete[] tree;
	Core::ShutDown();
	return 0;
}
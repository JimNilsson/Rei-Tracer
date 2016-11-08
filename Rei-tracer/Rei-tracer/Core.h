#ifndef _CORE_H_
#define _CORE_H_
#include <SDL.h>
#include <vector>
#include "Window.h"
#include "Direct3D11.h"
#include "CameraManager.h"
#include "Timer.h"
#include "InputManager.h"

//Singleton
class Core
{
private:
	Core();
	~Core();
	static Core* _instance;
	Window* _window;
	IGraphics* _graphics;
	CameraManager* _cameraManager;
	InputManager* _inputManager;

	Timer* _timer;

public:
	static void CreateInstance();
	static Core* GetInstance();
	static void ShutDown();
	void Init(uint32_t width, uint32_t height, bool fullscreen);
	void Update();

	Window* GetWindow() const;
	IGraphics* GetGraphics() const;
	CameraManager* GetCameraManager() const;
	Timer* GetTimer() const;
	InputManager* GetInputManager() const;



	
};


#endif

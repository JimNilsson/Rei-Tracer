#include "Core.h"
#include "Macros.h"
#include <exception>
#include <map>

Core* Core::_instance = nullptr;

Core::Core()
{
	_window = nullptr;
	_graphics = nullptr;
	_cameraManager = nullptr;
	_timer = nullptr;
}
Core::~Core()
{

}

void Core::CreateInstance()
{
	if (!_instance)
		_instance = new Core();
}
Core* Core::GetInstance()
{
	return _instance;
}

void Core::ShutDown()
{
	SAFE_DELETE(Core::GetInstance()->_graphics);
	SAFE_DELETE(Core::GetInstance()->_window);
	SAFE_DELETE(Core::GetInstance()->_cameraManager);
	SAFE_DELETE(Core::GetInstance()->_timer);
	SAFE_DELETE(Core::GetInstance()->_inputManager);
	delete _instance;
	_instance = nullptr;
}

void Core::Init(uint32_t width, uint32_t height, bool fullscreen)
{
	_window = new Window();
	_graphics = new Direct3D11();
	_cameraManager = new CameraManager();
	_timer = new Timer();
	_inputManager = new InputManager();

}

void Core::Update()
{
	_inputManager->Update();
	_timer->Update();
	_graphics->Draw();
}

Window * Core::GetWindow() const
{
	return _window;
}



IGraphics * Core::GetGraphics() const
{
	return _graphics;
}

CameraManager * Core::GetCameraManager() const
{
	return _cameraManager;
}

Timer * Core::GetTimer() const
{
	return _timer;
}

InputManager * Core::GetInputManager() const
{
	return _inputManager;
}


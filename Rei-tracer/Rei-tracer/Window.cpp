#include "Window.h"
#include <exception>
#include <SDL_syswm.h>
Window::Window(uint32_t width, uint32_t height, bool fullscreen)
{
	_width = width;
	_height = height;
	//Set up window and rendering context
	_window = nullptr;
	_surface = nullptr;
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		throw std::exception("Could not initialize SDL");
	_window = SDL_CreateWindow("Assignment 2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
	if (_window == nullptr)
		throw std::exception("Failed to create window");
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(_window, &info);
	_hwnd = info.info.win.window;
}

Window::~Window()
{
	SDL_Quit();
}

uint32_t Window::GetWidth() const
{
	return _width;
}

uint32_t Window::GetHeight() const
{
	return _height;
}

bool Window::GetWindowedMode() const
{
	return !fullScreen;
}

HWND Window::GetHandle() const
{
	return _hwnd;
}

void Window::SetTitle(const std::string & title)
{
	
	SDL_SetWindowTitle(_window, title.c_str());
}

void Window::LockMouseToScreen(bool lock)
{
	SDL_SetRelativeMouseMode((SDL_bool)lock);
}

void Window::ToggleLockMouseToScreen()
{
	if(SDL_GetRelativeMouseMode())
		SDL_SetRelativeMouseMode(SDL_bool::SDL_FALSE);
	else
		SDL_SetRelativeMouseMode(SDL_bool::SDL_TRUE);

}

//void Window::KeepMouseCentered(bool center)
//{
//	
//}

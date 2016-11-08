#ifndef _WINDOW_H_
#define _WINDOW_H_
#include <SDL.h>
#include <d3d11.h>
#include <string>
class Window
{
private:
	uint32_t _width;
	uint32_t _height;
	bool fullScreen;
	SDL_Window* _window;
	SDL_Surface* _surface;
	HWND _hwnd;

public:
	Window(uint32_t width = 800, uint32_t height = 600, bool fullscreen = false);
	~Window();

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	bool GetWindowedMode() const;
	HWND GetHandle() const;

	void SetTitle(const std::string& title);

	void LockMouseToScreen(bool lock);
	void ToggleLockMouseToScreen();
	//void KeepMouseCentered(bool center);
	

};


#endif

#include "InputManager.h"
#include <unordered_map>
#include <SDL_events.h>

InputManager::InputManager()
{
	_curX = 0;
	_curY = 0;
}

InputManager::~InputManager()
{
}

bool InputManager::WasKeyPressed(unsigned keyCode) const
{
	auto got = _pressedKeys.find(keyCode);
	if (got == _pressedKeys.end())
		return false;

	return got->second;
}

bool InputManager::IsKeyDown(unsigned keyCode) const
{
	auto got = _downKeys.find(keyCode);
	if (got == _downKeys.end())
		return false;

	return got->second;
}

int InputManager::GetMouseXMovement() const
{
	return _relX;
}
int InputManager::GetMouseYMovement() const
{
	return _relY;
}


void InputManager::Update()
{
	_pressedKeys.clear();
	_relX = 0;
	_relY = 0;
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
		case SDL_KEYUP:
		{
			_downKeys[ev.key.keysym.sym] = false;
			_pressedKeys[ev.key.keysym.sym] = false;
			break;
		}
		case SDL_KEYDOWN:
		{
			_pressedKeys[ev.key.keysym.sym] = true;
			_downKeys[ev.key.keysym.sym] = true;
			break;
		}
		case SDL_MOUSEMOTION:
		{
			_relX = ev.motion.xrel;
			_relY = ev.motion.yrel;
			_curX = ev.motion.x;
			_curY = ev.motion.y;
		}
		default:
			break;
		}
	}
	//SDL_GetMouseState(&_curX, &_curY);
}




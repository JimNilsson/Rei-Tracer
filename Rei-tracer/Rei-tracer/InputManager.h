#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_

#include "Structs.h"
#include <unordered_map>
#include <map>
class InputManager
{
public:
	InputManager();
	~InputManager();
	void Update();
	bool WasKeyPressed(unsigned keyCode) const;
	bool IsKeyDown(unsigned keyCode) const;
	int GetMouseXMovement() const;
	int GetMouseYMovement() const;
private:
	std::map<unsigned, bool> _pressedKeys;
	std::map<unsigned, bool> _downKeys;
	int _curX;
	int _curY;
	int _relX;
	int _relY;
};



#endif


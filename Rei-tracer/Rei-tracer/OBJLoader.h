#ifndef _OBJLOADER_H_
#define _OBJLOADER_H_
#include <fstream>
#include <iostream>
#include <vector>
#include <DirectXMath.h>
#include "Structs.h"
#include <string>

class OBJLoader
{
public:
	OBJLoader() {};
	~OBJLoader() {};

	unsigned LoadOBJ(const std::string& filename, Triangle* triangleArray, unsigned maxCount) const;
};


#endif


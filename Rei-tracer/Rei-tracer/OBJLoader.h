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
	//Returns the number of triangles added
	unsigned LoadOBJ(const std::string& filename, Triangle* triangleArray, unsigned maxCount) const;

	/*Partitions the mesh into an octree and sorts the triangle array accordingly.
	 *Will create new triangles where triangles overlap two nodes so that one triangle
	 *only exists in one node */
	unsigned PartitionMesh(Triangle* triangles, unsigned triangleCount, unsigned offset, OctNode** octTree, unsigned levels, unsigned& nodeCountOut, unsigned maxCount) const;

private:
	void _BuildOctTree(OctNode* octTree, unsigned index, unsigned stop, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 halflengths) const;
	bool _NodeContainsTriangle(const OctNode& node, const Triangle& triangle) const;

};


#endif


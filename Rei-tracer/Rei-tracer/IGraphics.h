#ifndef _IGRAPHICS_H_
#define _IGRAPHICS_H_
#include "Structs.h"

class IGraphics
{
public:
	IGraphics() {};
	virtual ~IGraphics() {};

	virtual void IncreaseBounceCount() = 0;
	virtual void DecreaseBounceCount() = 0;
	virtual void SetBounceCount(unsigned bounces) = 0;
	virtual void SetTriangles(Triangle* triangles, size_t count) = 0;
	virtual void SetSpheres(Sphere* spheres, size_t count) = 0;
	virtual void SetPointLights(PointLight* pointlights, size_t count) = 0;
	virtual void SetTextures(unsigned indexStart, unsigned indexCount, const std::string& filenameDiffuse, const std::string& filenameNormal) = 0;
	virtual void Draw() = 0;
	//CreateBuffer(Resource* ) is too generic to work. Depending on what kind of buffers/shader resource views need to be created
	//"Resource" needs to be able to hold a lot of different data structures which makes a fucking mess.
//	virtual void CreateMeshBuffers(const SM_GUID& guid, MeshData::Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t indexCount) = 0;
	//virtual void CreateShaderResource(const SM_GUID& guid, const void* data, uint32_t size) = 0;
	//virtual void AddToRenderQueue(const GameObject& gameObject) = 0;

};

#endif

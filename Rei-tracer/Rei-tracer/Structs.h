#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include <DirectXMath.h>
#include <vector>
#include <unordered_map>

struct Sphere
{
	Sphere() {};
	Sphere(float x, float y, float z, float radius)
	{
		posx = x; posy = y; posz = z; this->radius = radius;
	};
	float posx, posy, posz;
	float radius;
};

struct Plane
{
	Plane() {};
	Plane(float x, float y, float z, float d)
	{
		this->x = x; this->y = y; this->z = z; this->d = d;
	}
	float x, y, z;
	float d;
};

struct PointLight
{
	PointLight() {};
	PointLight(float x, float y, float z, float intensity, float red, float green, float blue, float range)
	{
		posx = x; posy = y; posz = z; this->intensity = intensity; this->red = red; this->green = green; this->blue = blue; this->range = range;
	};
	float posx, posy, posz;
	float intensity;
	float red, green, blue;
	float range;
};

struct TriangleVertex
{
	TriangleVertex() {};
	TriangleVertex(float px, float py, float pz, float u, float nx, float ny, float nz, float v, float tx, float ty, float tz, float handed)
	{
		posx = px; posy = py; posz = pz; this->u = u; norx = nx; nory = ny; norz = nz, this->v = v; tanx = tx; tany = ty; tanz = tz; handedness = handed;
	};
	float posx, posy, posz;
	float u;
	float norx, nory, norz;
	float v;
	float tanx, tany, tanz, handedness;
};

struct Triangle
{
	Triangle() {};
	Triangle(TriangleVertex v1, TriangleVertex v2, TriangleVertex v3)
	{
		this->v1 = v1;
		this->v2 = v2;
		this->v3 = v3;
	}
	TriangleVertex v1;
	TriangleVertex v2;
	TriangleVertex v3;
};

struct PNTVertex
{
	float posx, posy, posz;
	float norx, nory, norz;
	float texu, texv;
};

struct PNTMeshData
{
	PNTVertex* vertices;
	uint32_t vertexCount;
	uint32_t* indices;
	uint32_t indexCount;
};


struct Material
{
	float roughness = 1.0f;
	float metallic = 1.0f;
};


struct Camera
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT3 up;
	float fov;
	float aspectRatio;
	float nearPlane;
	float farPlane;

	DirectX::XMFLOAT3 GetRight() const
	{
		 
		DirectX::XMVECTOR f = DirectX::XMLoadFloat3(&forward);
		DirectX::XMVECTOR u = DirectX::XMLoadFloat3(&up);
		DirectX::XMFLOAT3 right;
		DirectX::XMStoreFloat3(&right, DirectX::XMVector3Cross(u, f));
		return right;
	}
};

struct PerFrameBuffer
{
	DirectX::XMFLOAT4X4 View;
	DirectX::XMFLOAT4X4 Proj;
	DirectX::XMFLOAT4X4 ViewProj;
	DirectX::XMFLOAT4X4 InvView;
	DirectX::XMFLOAT4X4 InvViewProj;
	DirectX::XMFLOAT4X4 InvProj;
	DirectX::XMFLOAT4 CamPos;
};

struct PerObjectBuffer
{
	DirectX::XMFLOAT4X4 WVP;
	DirectX::XMFLOAT4X4 WorldViewInvTrp;
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 WorldView;
};




enum Components
{
	TRANSFORM,
	MESH,
	TERRAIN,
	MATERIAL,
	TEXTURES,
	LIGHTSOURCE,
	COMPONENT_COUNT
};



#endif


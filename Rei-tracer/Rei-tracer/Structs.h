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


#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#define GBUFFER_COUNT 4
#define SAFE_RELEASE(x) {if(x){ x->Release(); x = nullptr;}};
#define MAX_INSTANCES 32 //If you change this, also change it in InstancedStaticMeshVS.hlsl

#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <map>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")
#include <mutex>

#include "Structs.h"
#include "IGraphics.h"
#include "ComputeHelp.h"
#include "D3D11Timer.h"

enum ConstantBuffers
{
	CB_PER_FRAME,
	CB_PER_OBJECT,
	CB_PER_INSTANCE,
	CB_LIGHTBUFFER,
	CB_COMPUTECONSTANTS,
	CB_COUNT
};

enum Samplers
{
	ANISO,
	SAM_COUNT
};

enum RasterizerStates
{
	RS_CULL_NONE,
	RS_CULL_BACK,
	RS_WIREFRAME,
	RS_COUNT
};

struct StructuredBuffer
{
	ID3D11Buffer* buffer = nullptr;
	ID3D11ShaderResourceView* srv = nullptr;
	ID3D11UnorderedAccessView* uav = nullptr;

	unsigned int stride = 0;
	unsigned int count = 0;
	~StructuredBuffer()
	{
		SAFE_RELEASE(buffer);
		SAFE_RELEASE(srv);
		SAFE_RELEASE(uav);
	}
};

struct ComputeConstants
{
	int gSphereCount = -1;
	int gPlaneCount = -1;
	int gOBBCount = -1;
	int gPointLightCount = -1;
};

enum StructuredBuffers
{
	SB_SPHERES,
	SB_PLANES,
	SB_OBBS,
	SB_TRIANGLES,
	SB_COUNT
};



class Direct3D11 : public IGraphics
{
private:
	ID3D11Device*                       _device = nullptr;
	ID3D11DeviceContext*                _deviceContext = nullptr;
	IDXGISwapChain*                     _swapChain = nullptr;
	ID3D11UnorderedAccessView*			_backBufferUAV = nullptr;

	ComputeWrap*						_computeWrap = nullptr;
	ComputeShader*						_computeShader = nullptr;

	D3D11Timer*							_timer = nullptr;
	
	ID3D11Buffer* _constantBuffers[ConstantBuffers::CB_COUNT] = { nullptr };
	ID3D11SamplerState* _samplerStates[Samplers::SAM_COUNT] = { nullptr };
	StructuredBuffer* _structuredBuffers[StructuredBuffers::SB_COUNT] = { nullptr };


	void _CreateSamplerState();
	void _CreateViewPort();

	void _CreateConstantBuffers();

	void _CreateStructuredBuffer(StructuredBuffer** buffer, unsigned int stride, unsigned int count, bool CPUWrite = true, bool GPUWrite = false, void* initdata = nullptr);


	ID3D11ShaderResourceView* _CreateDDSTexture(const void* data, size_t size);
	ID3D11ShaderResourceView* _CreateWICTexture(const void* data, size_t size);
	
	void _Map(ID3D11Resource* resource, void* data, uint32_t stride, uint32_t count, D3D11_MAP mapType, UINT flags);
		
	std::vector<Sphere> _spheres;
	std::vector<Plane> _planes;


public:
	Direct3D11();
	virtual ~Direct3D11();
	ID3D11Device* GetDevice() { return _device; }
	ID3D11DeviceContext* GetDeviceContext() { return _deviceContext; }

	


	//Inherited from graphics interface
	virtual void Draw();
	virtual void AddSphere(float posx, float posy, float posz, float radius);
	virtual void AddPlane(float x, float y, float z, float d);
//	virtual void CreateMeshBuffers(const SM_GUID& guid, MeshData::Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t indexCount);
//	virtual void CreateShaderResource(const SM_GUID& guid, const void* data, uint32_t size);
//	virtual void NotifyDelete(SM_GUID guid);
//	virtual void AddToRenderQueue(const GameObject& gameObject);

	
};


#endif


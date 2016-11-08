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
	


	void _CreateSamplerState();
	void _CreateViewPort();

	void _CreateConstantBuffers();

	ID3D11ShaderResourceView* _CreateDDSTexture(const void* data, size_t size);
	ID3D11ShaderResourceView* _CreateWICTexture(const void* data, size_t size);


public:
	Direct3D11();
	virtual ~Direct3D11();
	ID3D11Device* GetDevice() { return _device; }
	ID3D11DeviceContext* GetDeviceContext() { return _deviceContext; }

	


	//Inherited from graphics interface
	virtual void Draw();
//	virtual void CreateMeshBuffers(const SM_GUID& guid, MeshData::Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t indexCount);
//	virtual void CreateShaderResource(const SM_GUID& guid, const void* data, uint32_t size);
//	virtual void NotifyDelete(SM_GUID guid);
//	virtual void AddToRenderQueue(const GameObject& gameObject);

	
};


#endif


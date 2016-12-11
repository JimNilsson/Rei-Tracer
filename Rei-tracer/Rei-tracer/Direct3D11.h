#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#define GBUFFER_COUNT 4
#define SAFE_RELEASE(x) {if(x){ x->Release(); x = nullptr;}};
#define MAX_INSTANCES 32 //If you change this, also change it in InstancedStaticMeshVS.hlsl
#define MAX_TRIANGLES 2048
#define MAX_MESHTEXTURES 8
#define MAX_POINTLIGHTS 10
#define MAX_SPOTLIGHTS 10
#define MAX_OCTNODES_PER_MESH 1+8+64+512
#define MAX_MESHES 10

#define TEXTURE_DIMENSION 256U
#define TEXTURE_BYTESIZE 256U * 256U * 4U

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
	CB_COMPUTECAMERA,
	CB_COUNT
};

enum Samplers
{
	LINEAR,
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
	uint32_t gSphereCount = 0;
	uint32_t gTriangleCount = 0;
	uint32_t gPointLightCount = 0;
	int32_t gBounceCounts = 0;
	uint32_t gTextureCount = 0;
	int32_t gSpotLightCount = 0;
	int32_t gMeshIndexCount = 0;
	int32_t gPartitionCount = 0;
};

struct ComputeCamera
{
	DirectX::XMFLOAT3 position;
	float fardist;
	DirectX::XMFLOAT3 direction;
	float neardist;
	DirectX::XMFLOAT3 up;
	float aspectratio;
	DirectX::XMFLOAT3 right;
	float fov;
	int width;//Technically not based on camera
	int height;
	float pad0;
	float pad1;
};

enum StructuredBuffers
{
	SB_SPHERES,
	SB_TRIANGLES,
	SB_POINTLIGHTS,
	SB_TEXTUREOFFSETS,
	SB_SPOTLIGHTS,
	SB_MESHPARTITIONS,
	SB_MESHINDICES,
	SB_COUNT
};

struct TextureOffset
{
	unsigned begin;
	unsigned end;
	int diffuseIndex;
	int normalIndex;
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

	void _CreateWICTexture(const std::string& filename);
	
	void _Map(ID3D11Resource* resource, void* data, uint32_t stride, uint32_t count, D3D11_MAP mapType, UINT flags);
		
	std::vector<Sphere> _spheres;
	std::vector<Plane> _planes;
	std::vector<Triangle> _triangles;
	std::vector<TextureOffset> _triangleTextureOffsets;


	unsigned _bounceCount = 0;

	std::unordered_map<std::string, unsigned> _textureIndices;
	ID3D11ShaderResourceView* _textureArray = nullptr;

	bool _computeConstantsUpdated = false;
	ComputeConstants _computeConstants;

	uint8_t* _rawTextureData;

public:
	Direct3D11();
	virtual ~Direct3D11();
	ID3D11Device* GetDevice() { return _device; }
	ID3D11DeviceContext* GetDeviceContext() { return _deviceContext; }

	


	//Inherited from graphics interface
	virtual void Draw();

	//virtual void AddTriangleList(Triangle* triangles, size_t count);
	virtual void IncreaseBounceCount();
	virtual void DecreaseBounceCount();
	virtual void SetBounceCount(unsigned bounces);
	virtual void SetPointLights(PointLight* pointlights, size_t count);
	virtual void SetSpotLights(SpotLight* spotlights, size_t count);
	virtual void SetTriangles(Triangle* triangles, size_t count);
	virtual void SetSpheres(Sphere* spheres, size_t count);
	virtual void SetMeshPartitions(OctNode* nodes, MeshIndices* indices, size_t nodeCount, size_t indexCount);
	virtual void PrepareTextures(unsigned indexStart, unsigned indexEnd, const std::string& filenameDiffuse, const std::string& filenameNormal );
	virtual void SetTextures();

	
};


#endif


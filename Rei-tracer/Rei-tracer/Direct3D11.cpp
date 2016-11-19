#include "Direct3D11.h"
#include "Core.h"
#include "Structs.h"
#include <exception>
#include "DirectXTK\DDSTextureLoader.h"
#include "DirectXTK\WICTextureLoader.h"
#include <string>
#include <sstream>
#include <DirectXMath.h>
#include <algorithm>

using namespace DirectX;

Direct3D11::Direct3D11()
{
	const Core* core = Core::GetInstance();
	const Window* window = core->GetWindow();

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.Width = window->GetWidth();
	scd.BufferDesc.Height = window->GetHeight();
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
	scd.OutputWindow = core->GetWindow()->GetHandle();
	scd.SampleDesc.Count = 1; 
	scd.SampleDesc.Quality = 0;
	scd.Windowed = TRUE; 
	
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL, D3D11_SDK_VERSION, &scd, &_swapChain, &_device, NULL, &_deviceContext);
	if (FAILED(hr))
		throw std::exception("Failed to create device and swapchain");
	


	ID3D11Texture2D* backbuffer;
	_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer);
	hr = _device->CreateUnorderedAccessView(backbuffer, NULL, &_backBufferUAV);
	SAFE_RELEASE(backbuffer);

	_timer = new D3D11Timer(_device, _deviceContext);

	_computeWrap = new ComputeWrap(_device, _deviceContext);
	_computeShader = _computeWrap->CreateComputeShader(L"Shaders/raytracer.hlsl", NULL, "main", NULL);
	
	_CreateSamplerState();
	_CreateViewPort();
	_CreateConstantBuffers();
	_CreateStructuredBuffer(&_structuredBuffers[SB_SPHERES], sizeof(Sphere), 10);
	_CreateStructuredBuffer(&_structuredBuffers[SB_TRIANGLES], sizeof(Triangle), MAX_TRIANGLES);
	_triangles.reserve(MAX_TRIANGLES);
	_CreateStructuredBuffer(&_structuredBuffers[SB_PLANES], sizeof(Plane), 10);
	
}

Direct3D11::~Direct3D11()
{

	delete _timer;
	delete _computeWrap;
	delete _computeShader;

	for (auto &i : _samplerStates)
	{
		SAFE_RELEASE(i);
	}
	for (auto &i : _constantBuffers)
	{
		SAFE_RELEASE(i);
	}
	for (auto& i : _structuredBuffers)
	{
		SAFE_DELETE(i);
	}

	SAFE_RELEASE(_backBufferUAV);
	SAFE_RELEASE(_swapChain);

	SAFE_RELEASE(_deviceContext);
	if (_device)
	{
		uint32_t refCount = _device->Release();
		if (refCount > 0)
		{
			//DebugLog::PrintToConsole("Unreleased com objects: %d", refCount);
		}
	}

}



/* REWRITE TO CREATE FROM MEMORY */
ID3D11ShaderResourceView* Direct3D11::_CreateDDSTexture(const void* data, size_t size)
{
	ID3D11ShaderResourceView* srv = nullptr;

	HRESULT hr = CreateDDSTextureFromMemory(_device, (uint8_t*)data, size, nullptr, &srv);
	if (FAILED(hr))
	{
		return nullptr;
	}
	return srv;
}

ID3D11ShaderResourceView * Direct3D11::_CreateWICTexture(const void * data, size_t size)
{
	ID3D11ShaderResourceView* srv = nullptr;
	HRESULT hr = CreateWICTextureFromMemory(_device, (uint8_t*)data, size, nullptr, &srv);
	
	if (FAILED(hr))
	{
		return nullptr;
	}
	return srv;

}

void Direct3D11::_Map(ID3D11Resource * resource, void * data, uint32_t stride, uint32_t count, D3D11_MAP mapType, UINT flags)
{
	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT hr = _deviceContext->Map(resource, 0, mapType, 0, &map);
	memcpy(map.pData, data, stride * count);
	_deviceContext->Unmap(resource, 0);
}

void Direct3D11::Draw()
{
	const Core* core = Core::GetInstance();
	float clearColor[] = { 0.0f,0.0f,0.0f,0.0f };

	ID3D11UnorderedAccessView* uav[] = { _backBufferUAV };
	_deviceContext->CSSetUnorderedAccessViews(0, 1, uav, NULL);

	ID3D11Resource* resource = nullptr;
	if (_spheres.size())
	{
		_structuredBuffers[SB_SPHERES]->srv->GetResource(&resource);
		_Map(resource, &_spheres[0], _structuredBuffers[SB_SPHERES]->stride, min(_structuredBuffers[SB_SPHERES]->count, _spheres.size()), D3D11_MAP_WRITE_DISCARD, 0);
		SAFE_RELEASE(resource);
	}
	if (_triangles.size())
	{
		_structuredBuffers[SB_TRIANGLES]->srv->GetResource(&resource);
		_Map(resource, &_triangles[0], _structuredBuffers[SB_TRIANGLES]->stride, min(_structuredBuffers[SB_TRIANGLES]->count, _triangles.size()), D3D11_MAP_WRITE_DISCARD, 0);
		SAFE_RELEASE(resource);
	}
	if (_planes.size())
	{
		_structuredBuffers[SB_PLANES]->srv->GetResource(&resource);
		_Map(resource, &_planes[0], _structuredBuffers[SB_PLANES]->stride, min(_structuredBuffers[SB_PLANES]->count, _planes.size()), D3D11_MAP_WRITE_DISCARD, 0);
		SAFE_RELEASE(resource);
	}

	ComputeConstants cc;
	cc.gPlaneCount = _planes.size();
	cc.gTriangleCount = _triangles.size();
	cc.gSphereCount = _spheres.size();
	cc.gOBBCount = 0;
	cc.gPointLightCount = 0;

	_Map(_constantBuffers[ConstantBuffers::CB_COMPUTECONSTANTS], &cc, sizeof(cc), 1, D3D11_MAP_WRITE_DISCARD, 0);

	Camera cam = core->GetCameraManager()->GetActiveCamera();
	ComputeCamera ccam;
	ccam.position = cam.position;
	ccam.direction = cam.forward;
	ccam.right = cam.GetRight();
	ccam.up = cam.up;
	ccam.fardist = cam.farPlane;
	ccam.neardist = cam.nearPlane;
	ccam.height = core->GetWindow()->GetHeight();
	ccam.width = core->GetWindow()->GetWidth();
	ccam.fov = cam.fov;
	ccam.aspectratio = cam.aspectRatio;
	//ccam.position = XMFLOAT3(10.0f, 0.0f, 40.0f);
	//ccam.direction = XMFLOAT3(0.0f, 0.0f, -1.0f);
	//ccam.fardist = 50.0f;
	//ccam.neardist = 1.0f;
	//ccam.height = 400.0f;
	//ccam.width = 400.0f;
	//ccam.fov = XM_PI / 2.0f;
	//ccam.aspectratio = 1.0f;

	_Map(_constantBuffers[ConstantBuffers::CB_COMPUTECAMERA], &ccam, sizeof(ccam), 1, D3D11_MAP_WRITE_DISCARD, 0);
	

	_deviceContext->CSSetShaderResources(0, 1, &(_structuredBuffers[StructuredBuffers::SB_SPHERES]->srv));
	_deviceContext->CSSetShaderResources(1, 1, &(_structuredBuffers[StructuredBuffers::SB_TRIANGLES]->srv));
	_deviceContext->CSSetShaderResources(2, 1, &(_structuredBuffers[StructuredBuffers::SB_PLANES]->srv));
	_deviceContext->CSSetConstantBuffers(0, 1, &(_constantBuffers[ConstantBuffers::CB_COMPUTECAMERA]));
	_deviceContext->CSSetConstantBuffers(1, 1, &(_constantBuffers[ConstantBuffers::CB_COMPUTECONSTANTS]));

	_computeShader->Set();
	_timer->Start();
	_deviceContext->Dispatch(13, 13, 1);
	_timer->Stop();
	_computeShader->Unset();
	std::stringstream ss;
	ss << "FPS: " << static_cast<int>(10.0f / _timer->GetTime()) << "  Time to render: " << _timer->GetTime();

	Core::GetInstance()->GetWindow()->SetTitle(ss.str());

	if (FAILED(_swapChain->Present(0, 0)))
		return;
}

void Direct3D11::AddSphere(float posx, float posy, float posz, float radius)
{
	_spheres.push_back(std::move(Sphere(posx, posy, posz, radius)));
}

void Direct3D11::AddPlane(float x, float y, float z, float d)
{
	_planes.push_back(std::move(Plane(x, y, z, d)));
}



void Direct3D11::_CreateSamplerState()
{
	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.MaxAnisotropy = 16;
	sd.Filter = D3D11_FILTER_ANISOTROPIC;
	sd.MinLOD = -FLT_MAX;
	sd.MaxLOD = FLT_MAX;
	sd.MipLODBias = 0.0f;
	_device->CreateSamplerState(&sd, &_samplerStates[Samplers::ANISO]);
	_deviceContext->PSSetSamplers(0, 1, &_samplerStates[Samplers::ANISO]);
}

void Direct3D11::_CreateViewPort()
{
	const Core* core = Core::GetInstance();
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)core->GetWindow()->GetWidth();
	vp.Height = (FLOAT)core->GetWindow()->GetHeight();
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	_deviceContext->RSSetViewports(1, &vp);
}

void Direct3D11::_CreateConstantBuffers()
{
	HRESULT hr;
	D3D11_BUFFER_DESC bd;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.StructureByteStride = 0;
	bd.MiscFlags = 0;

	bd.ByteWidth = sizeof(PerFrameBuffer);
	_device->CreateBuffer(&bd, nullptr, &_constantBuffers[ConstantBuffers::CB_PER_FRAME]);

	bd.ByteWidth = sizeof(PerObjectBuffer);
	_device->CreateBuffer(&bd, nullptr, &_constantBuffers[ConstantBuffers::CB_PER_OBJECT]);

	bd.ByteWidth = sizeof(PerObjectBuffer) * MAX_INSTANCES;
	hr = _device->CreateBuffer(&bd, nullptr, &_constantBuffers[ConstantBuffers::CB_PER_INSTANCE]);

	bd.ByteWidth = sizeof(ComputeConstants);
	_device->CreateBuffer(&bd, nullptr, &_constantBuffers[CB_COMPUTECONSTANTS]);

	bd.ByteWidth = sizeof(ComputeCamera);
	_device->CreateBuffer(&bd, nullptr, &_constantBuffers[CB_COMPUTECAMERA]);


}

void Direct3D11::_CreateStructuredBuffer(StructuredBuffer ** buffer, unsigned int stride, unsigned int count, bool CPUWrite, bool GPUWrite, void * initdata)
{
	D3D11_BUFFER_DESC bd;
	bd.ByteWidth = stride * count;
	bd.StructureByteStride = stride;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	if (CPUWrite && !GPUWrite)
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else if (GPUWrite && !CPUWrite)
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		bd.CPUAccessFlags = 0;
	}
	else if (!CPUWrite && !GPUWrite)
	{
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bd.CPUAccessFlags = 0;
	}
	else
	{
		//Cant have both CPU and GPU write access
		throw std::exception("A structued buffer can't have both CPU and GPU write aceess");
	}

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = initdata; //nullptr unless otherwise specified
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	*buffer = new StructuredBuffer;
	(*buffer)->stride = stride;
	(*buffer)->count = count;

	HRESULT hr;
	if (initdata)
		hr = _device->CreateBuffer(&bd, &data, &((*buffer)->buffer));
	else
		hr = _device->CreateBuffer(&bd, nullptr, &((*buffer)->buffer));
	if (FAILED(hr))
	{
		delete *buffer;
		*buffer = nullptr;
		throw std::exception("Failed to create structured buffer");
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	srvd.Format = DXGI_FORMAT_UNKNOWN;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvd.Buffer.FirstElement = 0;
	srvd.Buffer.NumElements = count;

	hr = _device->CreateShaderResourceView((*buffer)->buffer, &srvd, &((*buffer)->srv));

	if (FAILED(hr))
	{
		delete *buffer;
		*buffer = nullptr;
		throw std::exception("Failed to create srv");
	}

	if (GPUWrite)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
		uavd.Format = DXGI_FORMAT_UNKNOWN;
		uavd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavd.Buffer.FirstElement = 0;
		uavd.Buffer.NumElements = count;
		uavd.Buffer.Flags = 0;

		hr = _device->CreateUnorderedAccessView((*buffer)->buffer, &uavd, &((*buffer)->uav));

		if (FAILED(hr))
		{
			delete *buffer;
			*buffer = nullptr;
			throw std::exception("Failed to create srv");
		}
	}
}

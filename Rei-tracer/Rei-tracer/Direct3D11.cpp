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

void Direct3D11::Draw()
{
	const Core* core = Core::GetInstance();
	float clearColor[] = { 0.0f,0.0f,0.0f,0.0f };

	ID3D11UnorderedAccessView* uav[] = { _backBufferUAV };
	_deviceContext->CSSetUnorderedAccessViews(0, 1, uav, NULL);

	_computeShader->Set();
	_timer->Start();
	_deviceContext->Dispatch(25, 25, 1);
	_timer->Stop();
	_computeShader->Unset();
	std::stringstream ss;
	ss << "Time to render: " << _timer->GetTime();

	Core::GetInstance()->GetWindow()->SetTitle(ss.str());

	if (FAILED(_swapChain->Present(0, 0)))
		return;
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


}

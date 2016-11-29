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
	
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, NULL, D3D11_SDK_VERSION, &scd, &_swapChain, &_device, NULL, &_deviceContext);
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
	_CreateStructuredBuffer(&_structuredBuffers[SB_POINTLIGHTS], sizeof(PointLight), MAX_POINTLIGHTS);
	_CreateStructuredBuffer(&_structuredBuffers[SB_TEXTUREOFFSETS], sizeof(TextureOffset), MAX_MESHTEXTURES);
	
	//Triangle ray test
	//XMVECTOR v1 = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	//XMVECTOR v2 = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	//XMVECTOR v3 = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);

	//XMVECTOR e1 = v2 - v1;
	//XMVECTOR e2 = v3 - v1;
	//XMVECTOR d = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	//XMVECTOR o = XMVectorSet(0.25f, 0.25f, -15.0f, 0.0f);
	//XMVECTOR q = XMVector3Cross(d, e2);
	//float a = XMVectorGetX(XMVector3Dot(e1, q));
	//float f = 1.0f / a;
	//XMVECTOR s = o - v1;
	//float u = f * XMVectorGetX(XMVector3Dot(s, q));
	//XMVECTOR r = XMVector3Cross(s, e1);
	//float v = f * XMVectorGetX(XMVector3Dot(d, r));
	//float distance = f * XMVectorGetX(XMVector3Dot(e2, r));
	//int ddd = 5;
	_rawTextureData = new uint8_t[256U * 256U * 4U * MAX_MESHTEXTURES * 2U];
	//HRESULT testerr = AppendTextureData(_rawTextureData, _device, L"testimage.png");
	//testerr = AppendTextureData(&_rawTextureData[256U*256U*4U], _device, L"rei.jpg");
	//CreateWICTextureFromFile(_device, L"testimage.png", nullptr, &_textures["testimage.png"].srv);

	// Create texture

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
	SAFE_RELEASE(_textureArray);
	if (_device)
	{
		uint32_t refCount = _device->Release();
		if (refCount > 0)
		{
			//DebugLog::PrintToConsole("Unreleased com objects: %d", refCount);
		}
	}
	delete _rawTextureData;

}


void Direct3D11::_CreateWICTexture(const std::string & filename)
{
	if (_textureIndices.size() == MAX_MESHTEXTURES)
		throw std::exception("Cannot create more textures");
	std::wstring name(filename.begin(), filename.end());
	uint8_t* destination = _rawTextureData + (_textureIndices.size() * TEXTURE_BYTESIZE);
	AppendTextureData(destination, _device, name.c_str());

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

	if (_computeConstantsUpdated)
	{
		_Map(_constantBuffers[ConstantBuffers::CB_COMPUTECONSTANTS], &_computeConstants, sizeof(ComputeConstants), 1, D3D11_MAP_WRITE_DISCARD, 0);
		_computeConstantsUpdated = false;
	}

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

	_Map(_constantBuffers[ConstantBuffers::CB_COMPUTECAMERA], &ccam, sizeof(ccam), 1, D3D11_MAP_WRITE_DISCARD, 0);
	
	//Set any textures we might have
	//_deviceContext->CSSetShaderResources

	_deviceContext->CSSetShaderResources(0, 1, &(_structuredBuffers[StructuredBuffers::SB_SPHERES]->srv));
	_deviceContext->CSSetShaderResources(1, 1, &(_structuredBuffers[StructuredBuffers::SB_TRIANGLES]->srv));
	_deviceContext->CSSetShaderResources(2, 1, &(_structuredBuffers[StructuredBuffers::SB_POINTLIGHTS]->srv));
	_deviceContext->CSSetShaderResources(3, 1, &(_structuredBuffers[StructuredBuffers::SB_TEXTUREOFFSETS]->srv));
	_deviceContext->CSSetShaderResources(4, 1, &_textureArray);

	_deviceContext->CSSetSamplers(0, 1, &_samplerStates[Samplers::LINEAR]);


	_deviceContext->CSSetConstantBuffers(0, 1, &(_constantBuffers[ConstantBuffers::CB_COMPUTECAMERA]));
	_deviceContext->CSSetConstantBuffers(1, 1, &(_constantBuffers[ConstantBuffers::CB_COMPUTECONSTANTS]));
	
	_computeShader->Set();
	_timer->Start();
	_deviceContext->Dispatch((ccam.width / 32) + ((ccam.width % 32) ? 1 : 0), (ccam.height / 32) + ((ccam.height % 32) ? 1 : 0), 1);
	_timer->Stop();
	_computeShader->Unset();
	_timer->GetTime();
	/*std::stringstream ss;
	ss << "FPS: " << static_cast<int>(10.0f / _timer->GetTime()) << "  Time to render: " << _timer->GetTime();

	Core::GetInstance()->GetWindow()->SetTitle(ss.str());*/

	if (FAILED(_swapChain->Present(0, 0)))
		return;
}


void Direct3D11::IncreaseBounceCount()
{
	_computeConstants.gBounceCounts = min(10, _computeConstants.gBounceCounts + 1);
	_computeConstantsUpdated = true;
}

void Direct3D11::DecreaseBounceCount()
{
	_computeConstants.gBounceCounts = max(0, _computeConstants.gBounceCounts - 1);//bouncecounts is unsigned so 0 - 1 will evaluate to 4 billion something.
	_computeConstantsUpdated = true;
}

void Direct3D11::SetBounceCount(unsigned bounces)
{
	_computeConstants.gBounceCounts = min(10, bounces);
	_computeConstantsUpdated = true;
}

void Direct3D11::SetPointLights(PointLight * pointlights, size_t count)
{
	ID3D11Resource* resource = nullptr;
	_structuredBuffers[SB_POINTLIGHTS]->srv->GetResource(&resource);
	_Map(resource, pointlights, _structuredBuffers[SB_POINTLIGHTS]->stride, min(_structuredBuffers[SB_POINTLIGHTS]->count, (uint32_t)count), D3D11_MAP_WRITE_DISCARD, 0);
	SAFE_RELEASE(resource);
	_computeConstants.gPointLightCount = min(_structuredBuffers[SB_POINTLIGHTS]->count, (uint32_t)count);
	_computeConstantsUpdated = true;
}

void Direct3D11::SetTriangles(Triangle * triangles, size_t count)
{
	ID3D11Resource* resource = nullptr;
	_structuredBuffers[SB_TRIANGLES]->srv->GetResource(&resource);
	_Map(resource, triangles, _structuredBuffers[SB_TRIANGLES]->stride, min(_structuredBuffers[SB_TRIANGLES]->count, (uint32_t)count), D3D11_MAP_WRITE_DISCARD, 0);
	SAFE_RELEASE(resource);
	_computeConstants.gTriangleCount = min(_structuredBuffers[SB_TRIANGLES]->count, (uint32_t)count);
	_computeConstantsUpdated = true;
}

void Direct3D11::SetSpheres(Sphere * spheres, size_t count)
{
	ID3D11Resource* resource = nullptr;
	_structuredBuffers[SB_SPHERES]->srv->GetResource(&resource);
	_Map(resource, spheres, _structuredBuffers[SB_SPHERES]->stride, min(_structuredBuffers[SB_SPHERES]->count, (uint32_t)count), D3D11_MAP_WRITE_DISCARD, 0);
	SAFE_RELEASE(resource);
	_computeConstants.gSphereCount = (int)min(_structuredBuffers[SB_SPHERES]->count, count);
	_computeConstantsUpdated = true;
}

void Direct3D11::PrepareTextures(unsigned indexStart, unsigned indexEnd, const std::string & filenameDiffuse, const std::string& filenameNormal)
{
	bool diffuse = true;
	bool normal = true;
	auto got = _textureIndices.find(filenameDiffuse);
	if (got == _textureIndices.end())
	{
		std::string fileend = filenameDiffuse.substr(filenameDiffuse.size() - 3);
		if (fileend == "png" || fileend == "jpg")
		{
			_CreateWICTexture(filenameDiffuse);
			size_t texindex = _textureIndices.size();
			_textureIndices[filenameDiffuse] = (unsigned)texindex;
		}
		else
			diffuse = false;
	}

	auto got2 = _textureIndices.find(filenameNormal);
	if (got2 == _textureIndices.end())
	{
		if (filenameNormal.size() > 3)
		{
			std::string fileend = filenameNormal.substr(filenameNormal.size() - 3);
			if (fileend == "png" || fileend == "jpg")
			{
				_CreateWICTexture(filenameNormal);
				size_t texindex = _textureIndices.size();
				_textureIndices[filenameNormal] = (unsigned)texindex;
			}
		}
		else
			normal = false;
	}
	//IF we cant create the textures, we set the index to -1 in the buffer supplied to the gpu

	//Check for exisiting entry
	bool found = false;
	for (auto& i : _triangleTextureOffsets)
	{
		if (i.begin == indexStart && i.end == indexEnd)
		{
			if (diffuse)
				i.diffuseIndex = _textureIndices[filenameDiffuse];
			else
				i.diffuseIndex = -1;
			if (normal)
				i.normalIndex = _textureIndices[filenameNormal];
			else
				i.normalIndex = -1;
			found = true;
			break;
		}
		//Check for invalid ranges, we cant have overlap
		else if ((indexStart < i.begin && indexEnd > i.begin) || (indexStart > i.begin && indexStart < i.end) || (indexStart == i.begin && indexEnd != i.end))
		{
			throw std::exception("Invalid range of triangles for texture. Conflicts with previous range.");
		}
	}
	if (!found)
	{
		TextureOffset to;
		to.begin = indexStart;
		to.end = indexEnd;
		if (diffuse)
			to.diffuseIndex = _textureIndices[filenameDiffuse];
		else
			to.diffuseIndex = -1;
		if (normal)
			to.normalIndex = _textureIndices[filenameNormal];
		else
			to.normalIndex = -1;

		_triangleTextureOffsets.push_back(to);
	}

	_computeConstants.gTextureCount = (int)_triangleTextureOffsets.size();
	_computeConstantsUpdated = true;
}

void Direct3D11::SetTextures()
{
	ID3D11Resource* resource = nullptr;
	_structuredBuffers[SB_TEXTUREOFFSETS]->srv->GetResource(&resource);
	_Map(resource, &_triangleTextureOffsets[0], _structuredBuffers[SB_TEXTUREOFFSETS]->stride, min(_structuredBuffers[SB_TEXTUREOFFSETS]->count, (uint32_t)_triangleTextureOffsets.size()), D3D11_MAP_WRITE_DISCARD, 0);
	SAFE_RELEASE(resource);

	HRESULT hr;
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = TEXTURE_DIMENSION;
	desc.Height = TEXTURE_DIMENSION;
	desc.MipLevels = 1;
	desc.ArraySize = (UINT)_textureIndices.size();
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA* initData = new D3D11_SUBRESOURCE_DATA[_textureIndices.size()];
	for (int i = 0; i < _textureIndices.size(); i++)
	{
		initData[i].pSysMem = &_rawTextureData[i * TEXTURE_BYTESIZE];
		initData[i].SysMemPitch = static_cast<UINT>(TEXTURE_DIMENSION * 4U);
		initData[i].SysMemSlicePitch = static_cast<UINT>(TEXTURE_BYTESIZE);
	}


	ID3D11Texture2D* tex = nullptr;

	hr = _device->CreateTexture2D(&desc, initData, &tex);
	if (SUCCEEDED(hr) && tex != 0)
	{
		if (&_textureArray != 0)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			memset(&SRVDesc, 0, sizeof(SRVDesc));
			SRVDesc.Format = desc.Format;

			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			SRVDesc.Texture2DArray.ArraySize = desc.ArraySize;
			SRVDesc.Texture2DArray.MipLevels = 1;
			SRVDesc.Texture2DArray.MostDetailedMip = 0;
			SRVDesc.Texture2DArray.FirstArraySlice = 0;

			hr = _device->CreateShaderResourceView(tex, &SRVDesc, &_textureArray);
			if (FAILED(hr))
			{
				tex->Release();
			}
		}

	}
	delete[] initData;
}



void Direct3D11::_CreateSamplerState()
{
	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.MaxAnisotropy = 0;
	sd.Filter = D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
	sd.MinLOD = -FLT_MAX;
	sd.MaxLOD = FLT_MAX;
	sd.MipLODBias = 0.0f;
	_device->CreateSamplerState(&sd, &_samplerStates[Samplers::LINEAR]);
	_deviceContext->PSSetSamplers(0, 1, &_samplerStates[Samplers::LINEAR]);
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

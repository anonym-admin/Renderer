#include "pch.h"
#include "FontManager.h"
#include "Renderer.h"

using namespace D2D1;

/*
===============
FontManager
===============
*/

FontManager::FontManager()
{
}

FontManager::~FontManager()
{
	CleanUp();
}

bool FontManager::Initialize(Renderer* renderer, ID3D12CommandQueue* cmdQueue, uint32 width, uint32 height, bool enableDebugLayer)
{
	ID3D12Device* device = renderer->GetDevice();
	
	if (!CreateD2D(device, cmdQueue, enableDebugLayer))
	{
		return false;
	}

	float dpi = renderer->GetDpi();

	if (!CreateDWrite(device, width, height, dpi))
	{
		return false;
	}

	return true;
}

void FontManager::CleanUp()
{
	if (m_d2dDeviceContext)
	{
		m_d2dDeviceContext->Release();
		m_d2dDeviceContext = nullptr;
	}
	if (m_d2dDevice)
	{
		m_d2dDevice->Release();
		m_d2dDevice = nullptr;
	}
}

bool FontManager::CreateD2D(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, bool enableDebugLayer)
{
	uint32 d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};

	ID2D1Factory3* d2dFactory = nullptr;
	ID3D11Device* d3d11Device = nullptr;
	ID3D11DeviceContext* d3d11DeviceContext = nullptr;
	ID3D11On12Device* d3d11On12Device = nullptr;
	IDXGIDevice* dxgiDevice = nullptr;

	if (enableDebugLayer)
	{
		d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
	d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

	ThrowIfFailed(D3D11On12CreateDevice(device, d3d11DeviceFlags, nullptr, 0, (IUnknown**)&cmdQueue, 1, 0, &d3d11Device, &d3d11DeviceContext, nullptr));
	ThrowIfFailed(d3d11Device->QueryInterface(IID_PPV_ARGS(&d3d11On12Device)));

	// Create D2D/DWrite components.
	D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, (void**)&d2dFactory));
	ThrowIfFailed(d3d11On12Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
	ThrowIfFailed(d2dFactory->CreateDevice(dxgiDevice, &m_d2dDevice));
	ThrowIfFailed(m_d2dDevice->CreateDeviceContext(deviceOptions, &m_d2dDeviceContext));

	if (dxgiDevice)
	{
		dxgiDevice->Release();
		dxgiDevice = nullptr;
	}
	if (d3d11On12Device)
	{
		d3d11On12Device->Release();
		d3d11On12Device = nullptr;
	}
	if (d3d11DeviceContext)
	{
		d3d11DeviceContext->Release();
		d3d11DeviceContext = nullptr;
	}
	if (d3d11Device)
	{
		d3d11Device->Release();
		d3d11Device = nullptr;
	}
	if (d2dFactory)
	{
		d2dFactory->Release();
		d2dFactory = nullptr;
	}

	return true;
}

bool FontManager::CreateDWrite(ID3D12Device* device, uint32 width, uint32 height, float dpi)
{
	return true;
}

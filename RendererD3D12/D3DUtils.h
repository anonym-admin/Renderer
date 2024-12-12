#pragma once

/*
==========================
HRESULT Check Function
==========================
*/
void ThrowIfFailed(HRESULT hr);

/*
==========
D3DUtils
==========
*/

class D3DUtils
{
public:
	static void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter = false);
	static void SetDebugLayerInfo(ID3D12Device* device);
	static void PrintError(ID3DBlob* error);
	static uint32 GetRequiredConstantDataSize(uint32 originSize);
	static void UpdateTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* texResource, ID3D12Resource* uploadBuufer);
};


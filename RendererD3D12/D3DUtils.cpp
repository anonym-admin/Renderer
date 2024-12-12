#include "pch.h"
#include "D3DUtils.h"

/*
==========================
HRESULT Check Function
==========================
*/
void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		__debugbreak();
	}
}

/*
==========
D3DUtils
==========
*/

void D3DUtils::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr;

	IDXGIAdapter1* adapter = nullptr;

	IDXGIFactory6* factory6 = nullptr;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (UINT adapterIndex = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(adapterIndex, requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&adapter))); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}
	if (adapter == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	factory6->Release();

	*ppAdapter = adapter;
	adapter = nullptr;
}

void D3DUtils::SetDebugLayerInfo(ID3D12Device* device)
{
	ID3D12InfoQueue* pInfoQueue = nullptr;
	device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
	if (pInfoQueue)
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			// Workarounds for debug layer issues on hybrid-graphics systems
			D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = (UINT)_countof(hide);
		filter.DenyList.pIDList = hide;
		pInfoQueue->AddStorageFilterEntries(&filter);

		pInfoQueue->Release();
		pInfoQueue = nullptr;
	}
}

void D3DUtils::PrintError(ID3DBlob* error)
{
	if (error)
	{
		printf("%s\n", (char*)error->GetBufferPointer());
		error->Release();
		__debugbreak();
	}
}

uint32 D3DUtils::GetRequiredConstantDataSize(uint32 originSize)
{
	return (originSize + 255) & ~255;
}

void D3DUtils::UpdateTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* texResource, ID3D12Resource* uploadBuufer)
{
	const uint32 MAX_SUBRESOURCE_NUM = 32;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrint[MAX_SUBRESOURCE_NUM];
	uint32 rows[MAX_SUBRESOURCE_NUM];
	uint64 rowSize[MAX_SUBRESOURCE_NUM];
	uint64 totalBytes = 0;

	D3D12_RESOURCE_DESC desc = texResource->GetDesc();

	device->GetCopyableFootprints(&desc, 0, desc.MipLevels, 0, footPrint, rows, rowSize, &totalBytes);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	for (uint32 i = 0; i < desc.MipLevels; i++)
	{
		D3D12_TEXTURE_COPY_LOCATION destLocation = {};
		destLocation.PlacedFootprint = footPrint[i];
		destLocation.pResource = texResource;
		destLocation.SubresourceIndex = i;
		destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION	srcLocation = {};
		srcLocation.PlacedFootprint = footPrint[i];
		srcLocation.pResource = uploadBuufer;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		cmdList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
}

#include "pch.h"
#include "Renderer.h"
#include "MeshObject.h"
#include "ResourceManager.h"
#include "ConstantBufferManager.h"
#include "DescriptorAllocator.h"
#include "DescriptorPool.h"

/*
=========
Renderer
=========
*/

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
	CleanUp();
}

bool Renderer::Initialize(HWND hwnd)
{
	ID3D12Debug* debugController = nullptr;
	IDXGIFactory4* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;

	// Debug layer.
	uint32 dxgiFactoryFlags = 0;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();

		// Enable additional debug layers.
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		ID3D12Debug5* debugController5 = nullptr;
		if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
		{
			debugController5->SetEnableGPUBasedValidation(true);
			debugController5->SetEnableAutoName(true);
			debugController5->Release();
		}
	}
	// Create DXGFactory.
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	// Create Device.
	DXGI_ADAPTER_DESC1 adapterDesc = {};
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	bool complete = false;
	uint32 numFeatureLevels = _countof(featureLevels);
	for (uint32 i = 0; i < numFeatureLevels; i++)
	{
		uint32 adapterIdx = 0;
		while (DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIdx, &adapter))
		{
			adapter->GetDesc1(&adapterDesc);
			if (SUCCEEDED(D3D12CreateDevice(adapter, featureLevels[i], IID_PPV_ARGS(&m_device))))
			{
				complete = true;
				break;
			}
			adapter->Release();
			adapter = nullptr;
			adapterIdx++;
		}

		if (complete)
		{
			break;
		}
	}

	if (!m_device)
	{
		__debugbreak();
		return false;
	}

	m_hwnd = hwnd;
	m_adapterDesc = adapterDesc;

	if (debugController)
	{
		D3DUtils::SetDebugLayerInfo(m_device);
	}

	// Create the descriptor allocator.
	m_descriptorAllocator = new DescriptorAllocator;
	m_descriptorAllocator->Initialize(m_device, MAX_DESCRIPTOR_COUNT);
	// Create the desciptor pool.
	m_descriptorPool = new DescriptorPool;
	m_descriptorPool->Initialize(m_device, MAX_DRAW_COUNT_PER_FRAME * MeshObject::MAX_DESCRIPTOR_COUNT_FOR_DRAW);
	// Create the resource manager.
	m_resourceManager = new ResourceManager;
	m_resourceManager->Initialize(m_device);
	// Create the constant buffer manager.
	m_constantBufferManager = new ConstantBufferManager;
	m_constantBufferManager->Initialize(m_device, MAX_DRAW_COUNT_PER_FRAME);

	// Create the command queue.
	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		cmdQueueDesc.NodeMask = 0;
		m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue));
	}

	RECT rect = {};
	::GetClientRect(hwnd, &rect);
	uint32 screenWidth = rect.right - rect.left;
	uint32 screenHeight = rect.bottom - rect.top;

	// Describe and create the swap chain.
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = Renderer::FRAME_COUNT;
		swapChainDesc.Width = static_cast<UINT>(screenWidth);
		swapChainDesc.Height = static_cast<UINT>(screenHeight);
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		m_swapChainFlag = static_cast<UINT>(swapChainDesc.Flags);
		// Full screen desc.
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = true;

		IDXGISwapChain1* swapChain = nullptr;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(m_cmdQueue, hwnd, &swapChainDesc, &fsSwapChainDesc, nullptr, &swapChain));
		swapChain->QueryInterface(IID_PPV_ARGS(&m_swapChain));
		swapChain->Release();
		swapChain = nullptr;
		m_frameIdx = m_swapChain->GetCurrentBackBufferIndex();
	}

	m_screenWidth = screenWidth;
	m_screenHeight = screenHeight;

	m_viewPort.Width = static_cast<float>(m_screenWidth);
	m_viewPort.Height = static_cast<float>(m_screenHeight);
	m_viewPort.MinDepth = 0.0f;
	m_viewPort.MaxDepth = 1.0f;

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = m_screenWidth;
	m_scissorRect.bottom = m_screenHeight;

	// Create rtv descriptor heap.
	CreateDescriptorHeapForRtv();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (uint32 i = 0; i < Renderer::FRAME_COUNT; i++)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
		m_device->CreateRenderTargetView(m_renderTargets[i], nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create dsv descriptor heap.
	CreateDescriptorHeapForDsv();
	CreateDepthStencilView(screenWidth, screenHeight);
	// Create the command list.
	CreateCommandList();
	// Create the fence.
	CreateFence();
	// Initialize the camera.
	InitCamera();

	if (adapter)
	{
		adapter->Release();
		adapter = nullptr;
	}
	if (factory)
	{
		factory->Release();
		factory = nullptr;
	}
	if (debugController)
	{
		debugController->Release();
		debugController = nullptr;
	}

	return true;
}

void Renderer::BeginRender()
{
	ThrowIfFailed(m_cmdAllocator->Reset());
	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator, nullptr));

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIdx, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	m_cmdList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	m_cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void Renderer::EndRender()
{
	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList* cmdLists[] = { m_cmdList };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

void Renderer::Present()
{
	Fence();

	uint32 syncInterval = m_syncInterval;
	uint32 presentInterval = 0;

	if (!syncInterval)
	{
		presentInterval |= DXGI_PRESENT_ALLOW_TEARING;
	}

	ThrowIfFailed(m_swapChain->Present(syncInterval, presentInterval));

	m_frameIdx = m_swapChain->GetCurrentBackBufferIndex();

	WaitForGpu(m_fenceValue);

	m_descriptorPool->Free();
	m_constantBufferManager->Free();
}

IT_MeshObject* Renderer::CreateMeshObject()
{
	MeshObject* meshObj = new MeshObject;
	meshObj->Initialize(this);
	return meshObj;
}

void* Renderer::CreateTextureFromFile(const wchar_t* filename)
{
	TEXTURE_HANDLE* textureHandle = nullptr;
	ID3D12Resource* texResource = nullptr;
	D3D12_RESOURCE_DESC resDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	m_resourceManager->CreateTextureFromFile(&texResource, &resDesc, filename);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = resDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

	m_descriptorAllocator->AllocateDescriptorHeap(&srv);
	m_device->CreateShaderResourceView(texResource, &srvDesc, srv);

	textureHandle = new TEXTURE_HANDLE;
	::memset(textureHandle, 0, sizeof(TEXTURE_HANDLE));
	textureHandle->textureResource = texResource;
	textureHandle->srv = srv;

	return textureHandle;
}

void Renderer::DestroyTexture(void* textureHandle)
{
	TEXTURE_HANDLE* texHandle = (TEXTURE_HANDLE*)textureHandle;

	if (texHandle)
	{
		if (texHandle->textureResource)
		{
			texHandle->textureResource->Release();
			texHandle->textureResource = nullptr;
		}
		if (texHandle->uploadBuffer)
		{
			texHandle->uploadBuffer->Release();
			texHandle->uploadBuffer = nullptr;
		}
		if (texHandle->srv.ptr)
		{
			m_descriptorAllocator->FreeDecriptorHeap(texHandle->srv);
		}

		delete texHandle;
	}
}

void Renderer::RenderMeshObject(IT_MeshObject* obj, Matrix worldRow)
{
	MeshObject* meshObj = (MeshObject*)obj;
	meshObj->Draw(m_cmdList, worldRow);
}

void Renderer::GetViewProjMatrix(Matrix* viewMat, Matrix* projMat)
{
	*viewMat = m_viewRow.Transpose();
	*projMat = m_projRow.Transpose();
}

void Renderer::InitCamera()
{
	m_camPos = Vector3(0.0f, 0.0f, -1.0f);
	m_camDir = Vector3(0.0f, 0.0f, 1.0f);
	SetCamera(m_camPos, m_camDir);
}

void Renderer::SetCameraPos(float x, float y, float z)
{
	m_camPos = Vector3(x, y, z);
	SetCamera(m_camPos, m_camDir);
}

void Renderer::SetCameraPos(Vector3 camPos)
{
	m_camPos = camPos;
	SetCamera(m_camPos, m_camDir);
}

void Renderer::GpuCompleted()
{
	WaitForGpu(m_fenceValue);
}

void Renderer::CleanUp()
{
	Fence();
	WaitForGpu(m_fenceValue);

	DestroyFence();
	DestroyCommandList();
	DestroyDepthStencilView();
	DestroyDescriptorHeapForDsv();

	for (uint32 i = 0; i < Renderer::FRAME_COUNT; i++)
	{
		if (m_renderTargets[i])
		{
			m_renderTargets[i]->Release();
			m_renderTargets[i] = nullptr;
		}
	}

	DestroyDescriptorHeapForRtv();

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = nullptr;
	}
	if (m_cmdQueue)
	{
		m_cmdQueue->Release();
		m_cmdQueue = nullptr;
	}
	if (m_constantBufferManager)
	{
		delete m_constantBufferManager;
		m_constantBufferManager = nullptr;
	}
	if (m_resourceManager)
	{
		delete m_resourceManager;
		m_resourceManager = nullptr;
	}
	if (m_descriptorPool)
	{
		delete m_descriptorPool;
		m_descriptorPool = nullptr;
	}
	if (m_descriptorAllocator)
	{
		delete m_descriptorAllocator;
		m_descriptorAllocator = nullptr;
	}
	if (m_device)
	{
		uint32 refCount = m_device->Release();
		if (refCount != 0)
		{
#ifdef _DEBUG
			IDXGIDebug1* debugController = nullptr;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debugController))))
			{
				debugController->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
				debugController->Release();
			}
#endif
		}
	}
}

HRESULT __stdcall Renderer::QueryInterface(REFIID riid, void** ppvObject)
{
	return E_NOTIMPL;
}

ULONG __stdcall Renderer::AddRef(void)
{
	uint32 refCount = ++m_refCount;
	return refCount;
}

ULONG __stdcall Renderer::Release(void)
{
	uint32 refCount = --m_refCount;
	if (refCount == 0)
	{
		delete this;
	}
	return refCount;
}

void Renderer::CreateDescriptorHeapForRtv()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = Renderer::FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void Renderer::CreateDescriptorHeapForDsv()
{
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

	m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void Renderer::CreateDepthStencilView(uint32 width, uint32 height)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_RESOURCE_DESC resDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		width,
		height,
		1,
		1,
		DXGI_FORMAT_R32_TYPELESS,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue, IID_PPV_ARGS(&m_depthStencilView)));
	ThrowIfFailed(m_depthStencilView->SetName(L"Renderer::depthStencilView"));

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateDepthStencilView(m_depthStencilView, &dsvDesc, dsvHandle);
}

void Renderer::CreateCommandList()
{
	// Create the command allocator.
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator)));
	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator, nullptr, IID_PPV_ARGS(&m_cmdList)));
	// Close.
	ThrowIfFailed(m_cmdList->Close());
}

void Renderer::CreateFence()
{
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

	m_fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
	if (!m_fenceEvent)
	{
		__debugbreak();
	}
}

void Renderer::DestroyDescriptorHeapForRtv()
{
	if (m_rtvHeap)
	{
		m_rtvHeap->Release();
		m_rtvHeap = nullptr;
	}
}

void Renderer::DestroyDescriptorHeapForDsv()
{
	if (m_dsvHeap)
	{
		m_dsvHeap->Release();
		m_dsvHeap = nullptr;
	}
}

void Renderer::DestroyCommandList()
{
	if (m_cmdList)
	{
		m_cmdList->Release();
		m_cmdList = nullptr;
	}
	if (m_cmdAllocator)
	{
		m_cmdAllocator->Release();
		m_cmdAllocator = nullptr;
	}
}

void Renderer::DestroyDepthStencilView()
{
	if (m_depthStencilView)
	{
		m_depthStencilView->Release();
		m_depthStencilView = nullptr;
	}
}

void Renderer::DestroyFence()
{
	if (m_fenceEvent)
	{
		::CloseHandle(m_fenceEvent);
	}
	if (m_fence)
	{
		m_fence->Release();
		m_fence = nullptr;
	}
}

void Renderer::Fence()
{
	uint64 curFenceValue = ++m_fenceValue;
	m_cmdQueue->Signal(m_fence, m_fenceValue);
}

void Renderer::WaitForGpu(uint64 expectedValue)
{
	uint64 completedValue = m_fence->GetCompletedValue();
	if (completedValue < expectedValue)
	{
		m_fence->SetEventOnCompletion(expectedValue, m_fenceEvent);
		::WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void Renderer::SetCamera(Vector3 camPos, Vector3 camDir)
{
	m_camPos = camPos;
	m_camDir = camDir;
	Vector3 up = Vector3(0.0f, 1.0f, 0.0);

	m_viewRow = XMMatrixLookToLH(m_camPos, m_camDir, up);

	constexpr float fov = XMConvertToRadians(120.0f);
	float aspect = GetAspectRatio();
	float nearZ = 0.01f;
	float farZ = 1000.0f;

	m_projRow = XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);
}

void Renderer::SetCamera(float x, float y, float z, float dirX, float dirY, float dirZ)
{
	m_camPos = Vector3(x, y, z);
	m_camDir = Vector3(dirX, dirY, dirZ);
	SetCamera(m_camPos, m_camDir);
}

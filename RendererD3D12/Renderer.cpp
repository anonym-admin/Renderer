#include "pch.h"
#include "Renderer.h"
#include "MeshObject.h"
#include "SpriteObject.h"
#include "FontManager.h"
#include "ResourceManager.h"
#include "ConstantBufferManager.h"
#include "TextureManager.h"
#include "DescriptorAllocator.h"
#include "DescriptorPool.h"
#include "RenderQueue.h"
#include "CommandContext.h"
#include "LineObject.h"

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

bool Renderer::Initialize(HWND hwnd, bool enableDebugLayer, bool enableGBV)
{
	ID3D12Debug* debugController = nullptr;
	IDXGIFactory4* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;

	// Debug layer.
	uint32 dxgiFactoryFlags = 0;
	if (enableDebugLayer)
	{
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

			if (enableGBV)
			{
				ID3D12Debug5* debugController5 = nullptr;
				if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
				{
					debugController5->SetEnableGPUBasedValidation(true);
					debugController5->SetEnableAutoName(true);
					debugController5->Release();
				}
			}
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
	m_dpi = static_cast<float>(::GetDpiForWindow(m_hwnd));
	m_adapterDesc = adapterDesc;

	if (debugController)
	{
		D3DUtils::SetDebugLayerInfo(m_device);
	}

	// Create the command queue.
	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		cmdQueueDesc.NodeMask = 0;
		m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue));
	}

	// Get physical core.
	uint32 physicalCoreCount = 0;
	uint32 logicalCoreCount = 0;
	GetPhysicalCoreCount(&physicalCoreCount, &logicalCoreCount);
	m_renderThreadCount = physicalCoreCount;
	if (m_renderThreadCount > MAX_THREAD_COUNT)
	{
		m_renderThreadCount = MAX_THREAD_COUNT;
	}
	// Create the thread elements.
#if MULTI_THREAD_RENDERING
	CreateThreadPool(m_renderThreadCount);
#endif
	// Create the descriptor allocator.
	m_descriptorAllocator = new DescriptorAllocator;
	m_descriptorAllocator->Initialize(m_device, MAX_DESCRIPTOR_COUNT);

	for (uint32 i = 0; i < FRAME_PENDING_COUNT; i++)
	{
		for (uint32 j = 0; j < m_renderThreadCount; j++)
		{
			// Create the desciptor pool.
			m_descriptorPool[i][j] = new DescriptorPool;
			m_descriptorPool[i][j]->Initialize(m_device, MAX_DRAW_COUNT_PER_FRAME * MeshObject::MAX_DESCRIPTOR_COUNT_FOR_DRAW);
			// Create the constant buffer manager.
			m_constantBufferManager[i][j] = new ConstantBufferManager;
			m_constantBufferManager[i][j]->Initialize(m_device, MAX_DRAW_COUNT_PER_FRAME);
			// Create the command context.
			m_cmdCtx[i][j] = new CommandContext;
			m_cmdCtx[i][j]->Initialize(m_device, 32);
		}
	}
	// Create the font manager.
	m_fontManager = new FontManager;
	m_fontManager->Initialize(this, m_cmdQueue, 1024, 256, enableDebugLayer);
	// Create the resource manager.
	m_resourceManager = new ResourceManager;
	m_resourceManager->Initialize(m_device);
	// Create the texture manager.
	m_textureManager = new TextureManager;
	m_textureManager->Initialize(this);
	// Create the render queue.
	for (uint32 i = 0; i < m_renderThreadCount; i++)
	{
		m_renderQueue[i] = new RenderQueue;
		m_renderQueue[i]->Initialize(m_device, 8192);
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

	// Create dsv descriptor heap.
	CreateDescriptorHeapForDsv();
	CreateDepthStencilView(screenWidth, screenHeight);
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
	CommandContext* cmdCtx = m_cmdCtx[m_framePendingIdx][0];
	ID3D12GraphicsCommandList* cmdList = cmdCtx->GetCurrentCommandList();

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIdx, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	//cmdList->RSSetViewports(1, &m_viewPort);
	//cmdList->RSSetScissorRects(1, &m_scissorRect);
	//cmdList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmdCtx->CloseAndExcute(m_cmdQueue);

	Fence();
}

void Renderer::EndRender()
{
	CommandContext* cmdCtx = m_cmdCtx[m_framePendingIdx][0];

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIdx, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

#if MULTI_THREAD_RENDERING
	m_activeThreadCount = m_renderThreadCount;
	for (uint32 i = 0; i < m_renderThreadCount; i++)
	{
		SetEvent(m_threadDesc[i].threadEvent[static_cast<uint32>(RENDER_THREAD_EVENT_TYPE::RENDER_THREAD_PROCESS)]);
	}
	WaitForSingleObject(m_completeThread, INFINITE);
#else
	for (uint32 i = 0; i < m_renderThreadCount; i++)
	{
		m_renderQueue[i]->Process(i, cmdCtx, m_cmdQueue, 400, rtvHandle, dsvHandle, m_viewPort, m_scissorRect);
	}
#endif

	ID3D12GraphicsCommandList* cmdList = cmdCtx->GetCurrentCommandList();
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	cmdCtx->CloseAndExcute(m_cmdQueue);
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

	uint32 framePendingIdx = (m_framePendingIdx + 1) % FRAME_PENDING_COUNT;

	WaitForGpu(m_fenceFramePendingValue[framePendingIdx]);

	for (uint32 threadIdx = 0; threadIdx < m_renderThreadCount; threadIdx++)
	{
		m_descriptorPool[framePendingIdx][threadIdx]->Free();
		m_constantBufferManager[framePendingIdx][threadIdx]->Free();
		m_cmdCtx[framePendingIdx][threadIdx]->Free();
		m_renderQueue[threadIdx]->Free();
	}

	m_framePendingIdx = framePendingIdx;
}

IT_MeshObject* Renderer::CreateMeshObject()
{
	MeshObject* meshObj = new MeshObject;
	meshObj->Initialize(this);
	return meshObj;
}

IT_SpriteObject* Renderer::CreateSpriteObject()
{
	SpriteObject* spriteObject = new SpriteObject;
	spriteObject->Initialize(this);
	return spriteObject;
}

IT_SpriteObject* Renderer::CreateSpriteObjectWithTexture(const wchar_t* filename, const RECT* rect)
{
	return nullptr;
}

IT_LineObject* Renderer::CreateLineObject()
{
	LineObject* lineObject = new LineObject;
	lineObject->Initialize(this);
	return lineObject;
}

void* Renderer::CreateFontObject(const wchar_t* fontName, float fontSize)
{
	void* fontHandle = m_fontManager->CreateFontObject(fontName, fontSize);
	if (!fontHandle)
	{
		__debugbreak();
	}

	return fontHandle;
}

void* Renderer::CreateTextureFromFile(const wchar_t* filename)
{
	void* handle = m_textureManager->CreateTextureFromFile(filename);
	if (!handle)
	{
		__debugbreak();
	}

	return handle;
}

void* Renderer::CreateTiledTexture(uint32 texWidth, uint32 texHeight, uint32 cellWidth, uint32 cellHeight)
{
	void* handle = m_textureManager->CreateTiledTexture(texWidth, texHeight, cellWidth, cellHeight);
	if (!handle)
	{
		__debugbreak();
	}

	return handle;
}

void* Renderer::CreateDynamicTexture(uint32 texWidth, uint32 texHeight, const char* name)
{
	void* handle = m_textureManager->CreateDynamicTexture(texWidth, texHeight, name);
	if (!handle)
	{
		__debugbreak();
	}

	return handle;
}

void* Renderer::CreateDummyTexture(uint32 texWidth, uint32 texHeight)
{
	void* handle = m_textureManager->CreateDummyTexture(texWidth, texHeight);
	if (!handle)
	{
		__debugbreak();
	}

	return handle;
}

void Renderer::WriteTextToBitmap(uint8* destImage, uint32 destWidth, uint32 destHeight, uint32 destPitch, int32* texWidth, int32* texHeight, void* fontHandle, const wchar_t* contentsString, uint32 strLen, FONT_COLOR_TYPE type)
{
	FONT_HANDLE* handle = reinterpret_cast<FONT_HANDLE*>(fontHandle);
	m_fontManager->WriteTextToBitmap(destImage, destWidth, destHeight, destPitch, texWidth, texHeight, handle, contentsString, strLen, type);
}

void Renderer::UpdateTextureWidthImage(void* textureHandle, const uint8* srcImage, uint32 srcWidth, uint32 srcHeight)
{
	TEXTURE_HANDLE* texHandle = reinterpret_cast<TEXTURE_HANDLE*>(textureHandle);
	ID3D12Resource* texResource = texHandle->textureResource;
	ID3D12Resource* upBuffer = texHandle->uploadBuffer;
	D3D12_RESOURCE_DESC texDesc = texResource->GetDesc();

	if (srcWidth > texDesc.Width)
	{
		__debugbreak();
	}
	if (srcHeight > texDesc.Height)
	{
		__debugbreak();
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrint = {};
	uint32 rows = 0;
	uint64 rowSize = 0;
	uint64 totalBytes = 0;

	m_device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footPrint, &rows, &rowSize, &totalBytes);

	uint8* mappedPtr = nullptr;
	CD3DX12_RANGE writeRange(0, 0);

	ThrowIfFailed(upBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&mappedPtr)));

	const uint8* src = srcImage;
	uint8* dest = mappedPtr;
	for (uint32 h = 0; h < srcHeight; h++)
	{
		memcpy(dest, src, srcWidth * 4);
		src += (srcWidth * 4);
		dest += footPrint.Footprint.RowPitch;
	}

	upBuffer->Unmap(0, nullptr);
}

void Renderer::DestroyFontObject(void* fontObj)
{
	if (fontObj)
	{
		FONT_HANDLE* fontHandle = reinterpret_cast<FONT_HANDLE*>(fontObj);
		m_fontManager->DestroyFontObject(fontHandle);
	}
}

void Renderer::DestroyTexture(void* textureHandle)
{
	TEXTURE_HANDLE* texHandle = reinterpret_cast<TEXTURE_HANDLE*>(textureHandle);

	m_textureManager->DestroyTexture(texHandle);
}

void Renderer::RenderMeshObject(IT_MeshObject* obj, Matrix worldRow)
{
	MeshObject* meshObj = reinterpret_cast<MeshObject*>(obj);

	RENDER_JOB job = {};
	job.type = RENDER_JOB_TYPE::RENDER_MESH_OBJECT;
	job.obj = meshObj;
	job.mesh.worldRow = worldRow;
	m_renderQueue[m_threadIdx]->Add(&job);

	m_threadIdx = (m_threadIdx + 1) % m_renderThreadCount;
}

void Renderer::RenderSpriteObject(IT_SpriteObject* obj, uint32 posX, uint32 posY, float scaleX, float scaleY, float z)
{
	SpriteObject* spriteObj = reinterpret_cast<SpriteObject*>(obj);

	RENDER_JOB job = {};
	job.type = RENDER_JOB_TYPE::RENDER_SPRITE_OBJECT;
	job.obj = spriteObj;
	job.sprite.posX = posX;
	job.sprite.posY = posY;
	job.sprite.scaleX = scaleX;
	job.sprite.scaleY = scaleY;
	job.sprite.z = z;
	m_renderQueue[m_threadIdx]->Add(&job);

	m_threadIdx = (m_threadIdx + 1) % m_renderThreadCount;
}

void Renderer::RenderSpriteObjectWithTexture(IT_SpriteObject* obj, uint32 posX, uint32 posY, float scaleX, float scaleY, float z, const RECT* rect, void* textureHandle, const char* name)
{
	SpriteObject* spriteObj = reinterpret_cast<SpriteObject*>(obj);
	TEXTURE_HANDLE* texHandle = reinterpret_cast<TEXTURE_HANDLE*>(textureHandle);

	RENDER_JOB job = {};
	job.type = RENDER_JOB_TYPE::RENDER_SPRITE_OBJECT;
	job.obj = spriteObj;
	job.sprite.posX = posX;
	job.sprite.posY = posY;
	job.sprite.scaleX = scaleX;
	job.sprite.scaleY = scaleY;
	job.sprite.z = z;
	job.sprite.rect = rect;
	job.sprite.texHandle = texHandle;
	strcpy_s(job.sprite.name, name);
	m_renderQueue[m_threadIdx]->Add(&job);

	m_threadIdx = (m_threadIdx + 1) % m_renderThreadCount;
}

void Renderer::RenderLineObject(IT_LineObject* obj, Matrix worldRow)
{
	LineObject* lineObject = reinterpret_cast<LineObject*>(obj);

	RENDER_JOB job = {};
	job.type = RENDER_JOB_TYPE::RENDER_LINE_OBJECT;
	job.obj = lineObject;
	job.mesh.worldRow = worldRow;
	m_renderQueue[m_threadIdx]->Add(&job);

	m_threadIdx = (m_threadIdx + 1) % m_renderThreadCount;
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

void Renderer::SetCameraRot(const float yaw, const float pitch, const float roll)
{
	Matrix rotY = Matrix::CreateRotationY(yaw);
	Matrix rotX = Matrix::CreateRotationX(pitch);
	m_camDir = Vector3::Transform(Vector3(0.0f, 0.0f, 1.0f), rotY);
	SetCamera(m_camPos, m_camDir);
}

void Renderer::SetCameraRot(const Vector3 dir)
{
	m_camDir = dir;
	SetCamera(m_camPos, m_camDir);
}

void Renderer::GpuCompleted()
{
	for (uint32 i = 0; i < FRAME_PENDING_COUNT; i++)
	{
		WaitForGpu(m_fenceFramePendingValue[i]);
	}
}

void Renderer::CleanUp()
{
	Fence();
	for (uint32 i = 0; i < FRAME_PENDING_COUNT; i++)
	{
		WaitForGpu(m_fenceFramePendingValue[i]);
	}

	DestroyFence();
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
	for (uint32 i = 0; i < m_renderThreadCount; i++)
	{
		if (m_renderQueue[i])
		{
			delete m_renderQueue[i];
			m_renderQueue[i] = nullptr;
		}
	}
	if (m_textureManager)
	{
		delete m_textureManager;
		m_textureManager = nullptr;
	}
	if (m_resourceManager)
	{
		delete m_resourceManager;
		m_resourceManager = nullptr;
	}
	if (m_fontManager)
	{
		delete m_fontManager;
		m_fontManager = nullptr;
	}
	for (uint32 i = 0; i < FRAME_PENDING_COUNT; i++)
	{
		for (uint32 j = 0; j < m_renderThreadCount; j++)
		{
			if (m_cmdCtx[i][j])
			{
				delete m_cmdCtx[i][j];
				m_cmdCtx[i][j] = nullptr;
			}
			if (m_constantBufferManager[i][j])
			{
				delete m_constantBufferManager[i][j];
				m_constantBufferManager[i][j] = nullptr;
			}
			if (m_descriptorPool[i])
			{
				delete m_descriptorPool[i][j];
				m_descriptorPool[i][j] = nullptr;
			}
		}
	}

	DestroyThreadPool();

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

void Renderer::CreateFence()
{
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

	m_fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
	if (!m_fenceEvent)
	{
		__debugbreak();
	}
}

void Renderer::CreateThreadPool(uint32 threadCount)
{
	m_threadDesc = new RENDER_THREAD_DESC[threadCount];

	for (uint32 i = 0; i < threadCount; i++)
	{
		for (uint32 j = 0; j < static_cast<uint32>(RENDER_THREAD_EVENT_TYPE::RENDER_THREAD_TYPE_COUNT); j++)
		{
			m_threadDesc[i].threadEvent[j] = CreateEvent(nullptr, false, false, nullptr);
		}
		uint32 threadId = 0;
		m_threadDesc[i].renderBody = this;
		m_threadDesc[i].threadIdx = i;
		m_threadDesc[i].threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, RenderThread::ProcessByRenderThread, m_threadDesc + i, 0, &threadId));
	}

	m_completeThread = CreateEvent(nullptr, false, false, nullptr);
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
		m_fenceEvent = nullptr;
	}
	if (m_fence)
	{
		m_fence->Release();
		m_fence = nullptr;
	}
}

void Renderer::DestroyThreadPool()
{
	if (m_threadDesc)
	{
		for (uint32 i = 0; i < m_renderThreadCount; i++)
		{
			SetEvent(m_threadDesc[i].threadEvent[static_cast<uint32>(RENDER_THREAD_EVENT_TYPE::RENDER_THREAD_EXIT)]);
			WaitForSingleObject(m_threadDesc[i].threadHandle, INFINITE);

			for (uint32 j = 0; j < static_cast<uint32>(RENDER_THREAD_EVENT_TYPE::RENDER_THREAD_TYPE_COUNT); j++)
			{
				if (m_threadDesc[i].threadEvent[j])
				{
					CloseHandle(m_threadDesc[i].threadEvent[j]);
				}
			}
			if (m_threadDesc[i].threadHandle)
			{
				CloseHandle(m_threadDesc[i].threadHandle);
			}
		}

		delete[] m_threadDesc;
		m_threadDesc = nullptr;
	}
}

void Renderer::Fence()
{
	uint64 curFenceValue = ++m_fenceValue;
	m_cmdQueue->Signal(m_fence, curFenceValue);
	m_fenceFramePendingValue[m_framePendingIdx] = curFenceValue;
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

	constexpr float fov = XMConvertToRadians(70.0f);
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

void Renderer::MoveRightCamera(float dt)
{
	Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 right = up.Cross(m_camDir);
	right.Normalize();
	m_camPos = m_camPos + dt * right;
	SetCamera(m_camPos, m_camDir);
}

void Renderer::MoveFrontCamera(float dt)
{
	m_camPos = m_camPos + dt * m_camDir;
	SetCamera(m_camPos, m_camDir);
}

void Renderer::MoveUpCamera(float dt)
{
	Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
	m_camPos = m_camPos + dt * up;
	SetCamera(m_camPos, m_camDir);
}

bool Renderer::MousePicking(DirectX::BoundingBox boundingBox, float ndcX, float ndcY, Vector3* hitPos, float* hitDist, float* ratio)
{
	Vector3 ndcNearPos = Vector3(ndcX, ndcY, 0.0f);
	Vector3 ndcFarPos = Vector3(ndcX, ndcY, 1.0f);

	Matrix invViewProj = (m_viewRow * m_projRow).Invert();

	Vector3 worldNearPos = Vector3::Transform(ndcNearPos, invViewProj);
	Vector3 worldFarPos = Vector3::Transform(ndcFarPos, invViewProj);

	Vector3 rayDir = worldFarPos - worldNearPos;
	float rayLength = rayDir.Length();
	rayDir.Normalize();

	DirectX::SimpleMath::Ray ray(worldNearPos, rayDir);
	float dist = 0.0f;
	if (ray.Intersects(boundingBox, dist))
	{
		*hitDist = dist;
		*ratio = dist / rayLength;
		*hitPos = worldNearPos + rayDir * dist;

		return true;
	}

	*hitDist = 0;
	*ratio = 0;
	*hitPos = Vector3(0.0f);

	return false;
}

void Renderer::MousePickingAfterMoveObject(float ndcX, float ndcY, Vector3* movePos, float ratio)
{
	Vector3 ndcNearPos = Vector3(ndcX, ndcY, 0.0f);
	Vector3 ndcFarPos = Vector3(ndcX, ndcY, 1.0f);

	Matrix invViewProj = (m_viewRow * m_projRow).Invert();

	Vector3 worldNearPos = Vector3::Transform(ndcNearPos, invViewProj);
	Vector3 worldFarPos = Vector3::Transform(ndcFarPos, invViewProj);

	*movePos = worldNearPos + ratio * (worldFarPos - worldNearPos);
}

uint32 Renderer::GetCmdListCount()
{
	uint32 totalCount = 0;
	for (uint32 i = 0; i < FRAME_PENDING_COUNT; i++)
	{
		for (uint32 j = 0; j < m_renderThreadCount; j++)
		{
			totalCount += m_cmdCtx[i][j]->GetCmdListCount();
		}
	}
	return totalCount;
}

/*
===================
Process
===================
*/

void Renderer::Process(uint32 threadIdx)
{
	CommandContext* cmdCtx = m_cmdCtx[m_framePendingIdx][threadIdx];

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIdx, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	m_renderQueue[threadIdx]->Process(threadIdx, cmdCtx, m_cmdQueue, 400, rtvHandle, dsvHandle, m_viewPort, m_scissorRect);

	uint64 curThreadCount = _InterlockedDecrement(&m_activeThreadCount);
	if (curThreadCount == 0)
	{
		SetEvent(m_completeThread);
	}
}

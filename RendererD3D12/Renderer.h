#pragma once

#include "../../Interface/IT_Renderer.h"

/*
=========
Renderer
=========
*/

class FontManager;
class ResourceManager;
class TextureManager;
class ConstantBufferManager;
class DescriptorAllocator;
class DescriptorPool;
class CommandContext;
class RenderQueue;

class Renderer : public IT_Renderer
{
public:
	static const uint32 FRAME_COUNT = 3;
	static const uint32 FRAME_PENDING_COUNT = 3;
	static const uint32 MAX_DESCRIPTOR_COUNT = 4096;
	static const uint32 MAX_DRAW_COUNT_PER_FRAME = 4096;

	Renderer();
	~Renderer();

	/*Interface*/
	virtual bool Initialize(HWND hwnd, bool enableDebugLayer, bool enableGBV) override;
	virtual void BeginRender() override;
	virtual void EndRender() override;
	virtual void Present() override;
	virtual IT_MeshObject* CreateMeshObject() override;
	virtual IT_SpriteObject* CreateSpriteObject() override;
	virtual IT_SpriteObject* CreateSpriteObjectWithTexture(const wchar_t* filename, const RECT* rect) override;
	virtual void* CreateFontObject(const wchar_t* fontName, float fontSize) override;
	virtual void* CreateTextureFromFile(const wchar_t* filename) override;
	virtual void* CreateTiledTexture(uint32 texWidth, uint32 texHeight, uint32 cellWidth, uint32 cellHeight) override;
	virtual void* CreateDynamicTexture(uint32 texWidth, uint32 texHeight) override;
	virtual void WriteTextToBitmap(uint8* destImage, uint32 destWidth, uint32 destHeight, uint32 destPitch, int32* texWidth, int32* texHeight, void* fontHandle, const wchar_t* contentsString, uint32 strLen) override;
	virtual void UpdateTextureWidthImage(void* textureHandle, const uint8* srcImage, uint32 srcWidth, uint32 srcHeight);
	virtual void DestroyFontObject(void* fontObj) override;
	virtual void DestroyTexture(void* textureHandle) override;
	virtual void RenderMeshObject(IT_MeshObject* obj, Matrix worldRow) override;
	virtual void RenderSpriteObject(IT_SpriteObject* obj, uint32 posX, uint32 posY, float scaleX, float scaleY, float z) override;
	virtual void RenderSpriteObjectWithTexture(IT_SpriteObject* obj, uint32 posX, uint32 posY, float scaleX, float scaleY, float z, const RECT* rect, void* textureHandle) override;
	virtual void SetCameraPos(float x, float y, float z) override;
	virtual void SetCameraPos(Vector3 camPos) override;
	virtual void SetCamera(Vector3 camPos, Vector3 camDir) override;
	virtual void SetCamera(float x, float y, float z, float dirX, float dirY, float dirZ) override;
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override;
	virtual ULONG STDMETHODCALLTYPE Release(void) override;

	/*Inline*/
	inline ID3D12Device5* GetDevice() { return m_device; }
	inline ResourceManager* GetReourceManager() { return m_resourceManager; }
	inline ConstantBufferManager* GetConstantBufferManager() { return m_constantBufferManager[m_framePendingIdx]; }
	inline DescriptorAllocator* GetDescriptorAllocator() { return m_descriptorAllocator; }
	inline DescriptorPool* GetDescriptorPool() { return m_descriptorPool[m_framePendingIdx]; }
	inline uint32 GetScreenWidth() { return m_screenWidth; }
	inline uint32 GetScreenHegiht() { return m_screenHeight; }
	inline float GetAspectRatio() { return static_cast<float>(m_screenWidth) / m_screenHeight; }
	inline float GetDpi() { return m_dpi; }

	/*DLL Inner*/
	void GetViewProjMatrix(Matrix* viewMat, Matrix* projMat);
	void InitCamera();
	void GpuCompleted();

private:
	void CleanUp();
	void CreateDescriptorHeapForRtv();
	void CreateDescriptorHeapForDsv();
	void CreateDepthStencilView(uint32 width, uint32 height);
	// void CreateCommandList();
	void CreateFence();
	void DestroyDescriptorHeapForRtv();
	void DestroyDescriptorHeapForDsv();
	// void DestroyCommandList();
	void DestroyDepthStencilView();
	void DestroyFence();
	void Fence();
	void WaitForGpu(uint64 expectedValue);

private:
	uint32 m_refCount = 1;
	HWND m_hwnd = nullptr;
	HANDLE m_fenceEvent = nullptr;
	ID3D12Device5* m_device = nullptr;
	ID3D12CommandQueue* m_cmdQueue = nullptr;
	DXGI_ADAPTER_DESC1 m_adapterDesc = {};
	IDXGISwapChain3* m_swapChain = nullptr;
	D3D12_VIEWPORT m_viewPort = {};
	D3D12_RECT m_scissorRect = {};
	ID3D12DescriptorHeap* m_rtvHeap = nullptr;
	ID3D12DescriptorHeap* m_dsvHeap = nullptr;
	ID3D12Resource* m_renderTargets[FRAME_COUNT] = {};
	ID3D12Resource* m_depthStencilView = nullptr;
	//ID3D12CommandAllocator* m_cmdAllocator[FRAME_PENDING_COUNT] = {};
	//ID3D12GraphicsCommandList* m_cmdList = nullptr;
	ID3D12Fence* m_fence = nullptr;
	uint32 m_swapChainFlag = 0;
	uint32 m_frameIdx = 0;
	uint32 m_framePendingIdx = 0;
	uint32 m_screenWidth = 0;
	uint32 m_screenHeight = 0;
	uint32 m_rtvDescriptorSize = 0;
	uint32 m_dsvDescriptorSize = 0;
	uint64 m_fenceValue = 0;
	uint64 m_fenceFramePendingValue[FRAME_PENDING_COUNT] = {};
	uint32 m_syncInterval = 0; // Vsync on:1/off:0
	Matrix m_viewRow = Matrix();
	Matrix m_projRow = Matrix();
	Vector3 m_camPos = Vector3(0.0f);
	Vector3 m_camDir = Vector3(0.0f);

	FontManager* m_fontManager = nullptr;
	ResourceManager* m_resourceManager = nullptr;
	TextureManager* m_textureManager = nullptr;
	ConstantBufferManager* m_constantBufferManager[FRAME_PENDING_COUNT] = {};
	DescriptorAllocator* m_descriptorAllocator = nullptr;
	DescriptorPool* m_descriptorPool[FRAME_PENDING_COUNT] = {};
	CommandContext* m_cmdCtx[FRAME_PENDING_COUNT] = {};
	float m_dpi = 0.0f;
};


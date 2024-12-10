#pragma once

#include "../../Interface/IT_Renderer.h"

/*
=========
Renderer
=========
*/

class Renderer : public IT_Renderer
{
public:
	Renderer();
	~Renderer();

	/*Interface*/
	virtual bool Initialize(HWND hwnd) override;
	virtual void BeginRender() override;
	virtual void EndRender() override;
	virtual void Present() override;

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override;
	virtual ULONG STDMETHODCALLTYPE Release(void) override;

	/*Inline*/
	inline ID3D12Device5* GetDevice() { return m_device; }

private:
	void CleanUp();
	void CreateDescriptorHeapForRtv();
	void CreateDescriptorHeapForDsv();
	void CreateDepthStencilView(uint32 width, uint32 height);
	void CreateCommandList();
	void CreateFence();
	void DestroyDescriptorHeapForRtv();
	void DestroyDescriptorHeapForDsv();
	void DestroyCommandList();
	void DestroyDepthStencilView();
	void DestroyFence();
	void Fence();
	void WaitForGpu(uint64 expectedValue);

private:
	const static uint32 FRAME_COUNT = 2;
	static uint32 sm_refCount;
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
	ID3D12CommandAllocator* m_cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* m_cmdList = nullptr;
	ID3D12Fence* m_fence = nullptr;
	uint32 m_swapChainFlag = 0;
	uint32 m_frameIdx = 0;
	uint32 m_screenWidth = 0;
	uint32 m_screenHeight = 0;
	uint32 m_rtvDescriptorSize = 0;
	uint32 m_dsvDescriptorSize = 0;
	uint32 m_srvDescriptorSize = 0;
	uint64 m_fenceValue = 0;
	uint32 m_syncInterval = 0;
};


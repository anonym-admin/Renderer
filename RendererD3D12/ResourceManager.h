#pragma once

/*
==================
ResourceManager
==================
*/

class Renderer;

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	bool Initialize(ID3D12Device5* device);
	void CleanUp();

	void CreateVertexBuffer(uint32 stride, uint32 numVertices, void* initData, D3D12_VERTEX_BUFFER_VIEW* vbView, ID3D12Resource** vertexBuffer);
	void CreateIndexBuffer(uint32 stride, uint32 numIndiecs, void* initData, D3D12_INDEX_BUFFER_VIEW* ibView, ID3D12Resource** indexBuffer);

private:
	void CreateCommandList();
	void CreateFence();

	void DestroyCommandList();
	void DestroyFence();

	void Fence();
	void WaitForGpu();

private:
	ID3D12Device5* m_device = nullptr;
	ID3D12CommandQueue* m_cmdQueue = nullptr;
	ID3D12CommandAllocator* m_cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* m_cmdList = nullptr;
	ID3D12Fence* m_fence = nullptr;
	HANDLE m_fenceEvent = nullptr;
	uint64 m_fenceValue = 0;
};


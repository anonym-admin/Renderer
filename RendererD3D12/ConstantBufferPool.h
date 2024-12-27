#pragma once

/*
===================
ConstantBufferPool
===================
*/

// class ConstantBuffer;

struct ConstantBuffer
{
	uint8* sysMemAddr = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS gpuMemAddr;
	D3D12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle = {};
};

class ConstantBufferPool
{
public:
	ConstantBufferPool();
	~ConstantBufferPool();

	bool Initialize(ID3D12Device5* device, uint32 cbTypeSize, uint32 maxNumCbv);
	void CleanUp();

	ConstantBuffer* Alloc();
	void Free();

private:
	ID3D12DescriptorHeap* m_descriptorHeap = nullptr;
	uint32 m_allocatedSize = 0;
	uint32 m_maxNumCbv = 0;
	uint32 m_heapTypeSize = 0;

	ID3D12Resource* m_cbResource = nullptr;
	ConstantBuffer* m_cb = nullptr;
	uint8* m_sysMemArr = nullptr;
};


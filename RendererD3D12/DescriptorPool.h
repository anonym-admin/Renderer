#pragma once

/*
===================
DescriptorPool
===================
*/

class DescriptorPool
{
public:
	DescriptorPool();
	~DescriptorPool();

	bool Initialize(ID3D12Device5* device, uint32 maxHeapNum);
	void CleanUp();

	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle, uint32 requiredSize);
	void Free();

	inline uint32 GetTypeSize() { return m_typeSize; }
	inline ID3D12DescriptorHeap* GetDesciptorHeap() { return m_descriptorHeap; }

private:
	ID3D12DescriptorHeap* m_descriptorHeap = nullptr;
	uint32 m_allocatedSize = 0;
	uint32 m_maxHeapNum = 0;
	uint32 m_typeSize = 0;
};


#pragma once

/*
===================
DesriptorAllocator
===================
*/

class DescriptorAllocator
{
public:
	DescriptorAllocator();
	~DescriptorAllocator();

	bool Initialize(ID3D12Device5* device, uint32 maxHeapNum);
	void CleanUp();

	void AllocateDescriptorHeap(D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle);
	void FreeDecriptorHeap(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleFromCpu(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

private:
	uint32 m_maxHeapNum = 0;
	uint32 m_allocatedHeapNum = 0;
	uint32 m_descriptorSize = 0;
	ID3D12DescriptorHeap* m_descriptorHeap = nullptr;
};


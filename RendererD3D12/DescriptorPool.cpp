#include "pch.h"
#include "DescriptorPool.h"

/*
===================
DescriptorPool
===================
*/

DescriptorPool::DescriptorPool()
{
}

DescriptorPool::~DescriptorPool()
{
	CleanUp();
}

bool DescriptorPool::Initialize(ID3D12Device5* device, uint32 maxHeapNum)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = maxHeapNum;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap)));

	m_allocatedSize = 0;
	m_maxHeapNum = maxHeapNum;
	m_typeSize = device->GetDescriptorHandleIncrementSize(heapDesc.Type);

	return true;
}

void DescriptorPool::CleanUp()
{
	if (m_descriptorHeap)
	{
		m_descriptorHeap->Release();
		m_descriptorHeap = nullptr;
	}
}

void DescriptorPool::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle, uint32 requiredSize)
{
	if (m_allocatedSize >= m_maxHeapNum)
	{
		__debugbreak();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpu(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_allocatedSize, m_typeSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), m_allocatedSize, m_typeSize);

	*cpuHandle = cpu;
	*gpuHandle = gpu;

	m_allocatedSize += requiredSize;
}

void DescriptorPool::Free()
{
	m_allocatedSize = 0;
}

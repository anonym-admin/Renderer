#include "pch.h"
#include "DescriptorAllocator.h"
#include "D3DUtils.h"

/*
===================
DesriptorAllocator
===================
*/

DescriptorAllocator::DescriptorAllocator()
{
}

DescriptorAllocator::~DescriptorAllocator()
{
	CleanUp();
}

bool DescriptorAllocator::Initialize(ID3D12Device5* device, uint32 maxHeapNum)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = maxHeapNum;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap)));

	m_maxHeapNum = maxHeapNum;
	m_allocatedHeapNum = 0;
	m_descriptorSize = device->GetDescriptorHandleIncrementSize(heapDesc.Type);

	return true;
}

void DescriptorAllocator::CleanUp()
{
	m_allocatedHeapNum = 0;
	m_maxHeapNum = 0;

	if (m_descriptorHeap)
	{
		m_descriptorHeap->Release();
		m_descriptorHeap = nullptr;
	}
}

void DescriptorAllocator::AllocateDescriptorHeap(D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle)
{
	if (m_allocatedHeapNum >= m_maxHeapNum)
	{
		__debugbreak();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_allocatedHeapNum, m_descriptorSize);
	*cpuHandle = descriptorHandle;
	m_allocatedHeapNum++;
}

void DescriptorAllocator::FreeDecriptorHeap(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
	m_allocatedHeapNum--;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetGpuHandleFromCpu(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE baseHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	uint32 offset = (uint32)(cpuHandle.ptr - baseHandle.ptr) / m_descriptorSize;

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), offset, m_descriptorSize);

	return gpuHandle;
}

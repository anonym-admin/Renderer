#include "pch.h"
#include "ConstantBufferPool.h"

/*
===================
ConstantBufferPool
===================
*/

ConstantBufferPool::ConstantBufferPool()
{
}

ConstantBufferPool::~ConstantBufferPool()
{
	CleanUp();
}

bool ConstantBufferPool::Initialize(ID3D12Device5* device, uint32 cbTypeSize, uint32 maxNumCbv)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = maxNumCbv;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap)));

	m_allocatedSize = 0;
	m_maxNumCbv = maxNumCbv;
	m_heapTypeSize = device->GetDescriptorHandleIncrementSize(heapDesc.Type);

	uint32 bufSize = cbTypeSize * m_maxNumCbv;
	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_cbResource)));

	CD3DX12_RANGE writeRange(0, 0);		// We do not intend to write from this resource on the CPU.
	m_cbResource->Map(0, &writeRange, reinterpret_cast<void**>(&m_sysMemArr));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc = {};
	cbViewDesc.BufferLocation = m_cbResource->GetGPUVirtualAddress();
	cbViewDesc.SizeInBytes = cbTypeSize;

	m_cb = new ConstantBuffer[maxNumCbv];

	uint8* sysMemAddr = m_sysMemArr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (uint32 i = 0; i < m_maxNumCbv; i++)
	{
		device->CreateConstantBufferView(&cbViewDesc, cbvHandle);

		m_cb[i].sysMemAddr = sysMemAddr;
		m_cb[i].gpuMemAddr = cbViewDesc.BufferLocation;
		m_cb[i].cbvCpuHandle = cbvHandle;

		sysMemAddr += cbTypeSize;
		cbViewDesc.BufferLocation += cbTypeSize;
		cbvHandle.Offset(1, m_heapTypeSize);
	}

	return true;
}

void ConstantBufferPool::CleanUp()
{
	if (m_cb)
	{
		delete[] m_cb;
		m_cb = nullptr;
	}
	if (m_cbResource)
	{
		m_cbResource->Release();
		m_cbResource = nullptr;
	}
	if (m_descriptorHeap)
	{
		m_descriptorHeap->Release();
		m_descriptorHeap = nullptr;
	}
}

ConstantBuffer* ConstantBufferPool::Alloc()
{
	if (m_allocatedSize >= m_maxNumCbv)
	{
		__debugbreak();
	}

	ConstantBuffer* cb = nullptr;

	cb = m_cb + m_allocatedSize;
	m_allocatedSize++;

	return cb;
}

void ConstantBufferPool::Free()
{
	m_allocatedSize = 0;
}

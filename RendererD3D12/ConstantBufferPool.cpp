#include "pch.h"
#include "ConstantBufferPool.h"
#include "ConstantBuffer.h"

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
	m_typeSize = device->GetDescriptorHandleIncrementSize(heapDesc.Type);

	m_constantBuffer = new ConstantBuffer * [m_maxNumCbv];
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());
	for (uint32 i = 0; i < m_maxNumCbv; i++)
	{
		m_constantBuffer[i] = new ConstantBuffer;
		m_constantBuffer[i]->Initialize(device, cbTypeSize, cbvHandle);
		cbvHandle.Offset(1, m_typeSize);
	}

	return true;
}

void ConstantBufferPool::CleanUp()
{
	if (m_constantBuffer)
	{
		for (uint32 i = 0; i < m_maxNumCbv; i++)
		{
			if (m_constantBuffer[i])
			{
				delete m_constantBuffer[i];
				m_constantBuffer[i] = nullptr;
			}
		}

		delete[] m_constantBuffer;
		m_constantBuffer = nullptr;
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

	ConstantBuffer* cb = m_constantBuffer[m_allocatedSize];
	m_allocatedSize++;

	return cb;
}

void ConstantBufferPool::Free()
{
	m_allocatedSize = 0;
}

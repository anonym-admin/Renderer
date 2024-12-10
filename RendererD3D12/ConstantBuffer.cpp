#include "pch.h"
#include "ConstantBuffer.h"
#include "D3DUtils.h"

/*
================
ConstantBuffer
================
*/

bool ConstantBuffer::Initialize(ID3D12Device5* device, uint32 typeSize, D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle)
{
    uint32 size = D3DUtils::GetRequiredConstantDataSize(typeSize);
    m_typeSize = size;

    ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer)));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc = {};
    cbViewDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
    cbViewDesc.SizeInBytes = size;

    device->CreateConstantBufferView(&cbViewDesc, cbvHandle);

    return true;
}

void ConstantBuffer::CleanUp()
{
    if (m_constantBuffer)
    {
        m_constantBuffer->Release();
        m_constantBuffer = nullptr;
    }
}

void ConstantBuffer::Upload(void* data)
{
    CD3DX12_RANGE readRange(0, 0);

    uint8* cbDataBegin = nullptr;
    ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&cbDataBegin)));
    memcpy(cbDataBegin, data, m_typeSize);
    m_constantBuffer->Unmap(0, nullptr);
}

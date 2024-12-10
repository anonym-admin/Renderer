#include "pch.h"
#include "ResourceManager.h"
#include "D3DUtils.h"

/*
==================
ResourceManager
==================
*/

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
    CleanUp();
}

bool ResourceManager::Initialize(ID3D12Device5* device)
{
    m_device = device;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_cmdQueue)));

    CreateCommandList();
    CreateFence();

    return true;
}

void ResourceManager::CleanUp()
{
    WaitForGpu();

    DestroyFence();
    DestroyCommandList();

    if (m_cmdQueue)
    {
        m_cmdQueue->Release();
        m_cmdQueue = nullptr;
    }
}

void ResourceManager::CreateVertexBuffer(uint32 stride, uint32 numVertices, void* initData, D3D12_VERTEX_BUFFER_VIEW* vbView, ID3D12Resource** vertexBuffer)
{
    D3D12_VERTEX_BUFFER_VIEW view = {};
    ID3D12Resource* vb = nullptr;
    ID3D12Resource* up = nullptr;
    uint32 size = stride * numVertices;

    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&vb)));

    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&up)));

    uint8* vbDataBegin = nullptr;
    CD3DX12_RANGE readRange(0, 0);

    ThrowIfFailed(up->Map(0, &readRange, reinterpret_cast<void**>(&vbDataBegin)));
    memcpy(vbDataBegin, initData, size);
    up->Unmap(0, nullptr);

    ThrowIfFailed(m_cmdAllocator->Reset());
    ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator, nullptr));

    m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vb, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    m_cmdList->CopyBufferRegion(vb, 0, up, 0, size);
    m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vb, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

    ThrowIfFailed(m_cmdList->Close());
    ID3D12CommandList* cmdLists[] = { m_cmdList };
    m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    Fence();
    WaitForGpu();

    view.BufferLocation = vb->GetGPUVirtualAddress();
    view.SizeInBytes = size;
    view.StrideInBytes = stride;

    if (up)
    {
        up->Release();
        up = nullptr;
    }

    *vertexBuffer = vb;
    *vbView = view;
}

void ResourceManager::CreateIndexBuffer(uint32 stride, uint32 numIndiecs, void* initData, D3D12_INDEX_BUFFER_VIEW* ibView, ID3D12Resource** indexBuffer)
{
    D3D12_INDEX_BUFFER_VIEW view = {};
    ID3D12Resource* ib = nullptr;
    ID3D12Resource* up = nullptr;
    uint32 size = stride * numIndiecs;

    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&ib)));

    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&up)));

    uint8* ibDataBegin = nullptr;
    CD3DX12_RANGE readRange(0, 0);

    ThrowIfFailed(up->Map(0, &readRange, reinterpret_cast<void**>(&ibDataBegin)));
    memcpy(ibDataBegin, initData, size);
    up->Unmap(0, nullptr);

    ThrowIfFailed(m_cmdAllocator->Reset());
    ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator, nullptr));

    m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ib, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    m_cmdList->CopyBufferRegion(ib, 0, up, 0, size);
    m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ib, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

    ThrowIfFailed(m_cmdList->Close());
    ID3D12CommandList* cmdLists[] = { m_cmdList };
    m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    Fence();
    WaitForGpu();

    view.BufferLocation = ib->GetGPUVirtualAddress();
    view.SizeInBytes = size;
    view.Format = DXGI_FORMAT_R32_UINT;

    if (up)
    {
        up->Release();
        up = nullptr;
    }

    *indexBuffer = ib;
    *ibView = view;
}

void ResourceManager::CreateCommandList()
{
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator)));

    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator, nullptr, IID_PPV_ARGS(&m_cmdList)));

    ThrowIfFailed(m_cmdList->Close());
}

void ResourceManager::CreateFence()
{
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

    m_fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
    if (!m_fenceEvent)
    {
        __debugbreak();
    }
}

void ResourceManager::DestroyCommandList()
{
    if (m_cmdList)
    {
        m_cmdList->Release();
        m_cmdList = nullptr;
    }
    if (m_cmdAllocator)
    {
        m_cmdAllocator->Release();
        m_cmdAllocator = nullptr;
    }
}

void ResourceManager::DestroyFence()
{
    if (m_fence)
    {
        m_fence->Release();
        m_fence = nullptr;
    }
}

void ResourceManager::Fence()
{
   uint64 curFenceValue = ++m_fenceValue;
    m_cmdQueue->Signal(m_fence, m_fenceValue);
}

void ResourceManager::WaitForGpu()
{
    uint64 expectedValue = m_fenceValue;
    uint64 completedValue = m_fence->GetCompletedValue();
    if (completedValue < expectedValue)
    {
        m_fence->SetEventOnCompletion(expectedValue, m_fenceEvent);
        ::WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

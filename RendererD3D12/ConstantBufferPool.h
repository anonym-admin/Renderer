#pragma once

/*
===================
ConstantBufferPool
===================
*/

class ConstantBuffer;

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
	ConstantBuffer** m_constantBuffer = nullptr;
	uint32 m_allocatedSize = 0;
	uint32 m_maxNumCbv = 0;
	uint32 m_typeSize = 0;
};


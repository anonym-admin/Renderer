#pragma once

/*
================
ConstantBuffer
================
*/

class ConstantBuffer
{
public:
	bool Initialize(ID3D12Device5* device, uint32 typeSize, D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle);
	void CleanUp();
	void Upload(void* data);

private:
	// ID3D12Device5* m_device = nullptr;

	ID3D12Resource* m_constantBuffer = nullptr;
	uint32 m_typeSize = 0;
};


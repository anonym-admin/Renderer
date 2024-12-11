#pragma once

/*
================
ConstantBuffer
================
*/

class ConstantBuffer
{
public:
	ConstantBuffer();
	~ConstantBuffer();

	bool Initialize(ID3D12Device5* device, uint32 typeSize, D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle);
	void CleanUp();
	void Upload(void* data);

	inline D3D12_CPU_DESCRIPTOR_HANDLE GetCbvHandle() { return m_cbvHandle; }

private:
	ID3D12Resource* m_constantBuffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cbvHandle = {};
	uint32 m_typeSize = 0;
};


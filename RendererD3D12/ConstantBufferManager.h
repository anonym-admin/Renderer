#pragma once

/*
=======================
ConstantBufferManager
=======================
*/

class ConstantBufferPool;

class ConstantBufferManager
{
public:
	ConstantBufferManager();
	~ConstantBufferManager();

	bool Initialize(ID3D12Device5* device, uint32 maxNumCbv);
	void CleanUp();

	void Free();

	ConstantBufferPool* GetConstantBufferPool(CONSTANT_BUFFER_TYPE type);

private:
	ConstantBufferPool* m_constantBufferPool[static_cast<uint32>(CONSTANT_BUFFER_TYPE::CONST_TYPE_COUNT)];
};


#include "pch.h"
#include "ConstantBufferManager.h"
#include "ConstantBufferPool.h"

/*
=======================
ConstantBufferManager
=======================
*/

uint32 CONST_TYPE_SIZE[] =
{
	sizeof(MESH_CONST_DATA),
	sizeof(SPRITE_CONST_DATA),
};

ConstantBufferManager::ConstantBufferManager()
{
}

ConstantBufferManager::~ConstantBufferManager()
{
	CleanUp();
}

bool ConstantBufferManager::Initialize(ID3D12Device5* device, uint32 maxNumCbv)
{
	for (uint32 i = 0; i < static_cast<uint32>(CONSTANT_BUFFER_TYPE::CONST_TYPE_COUNT); i++)
	{
		m_constantBufferPool[i] = new ConstantBufferPool;
		m_constantBufferPool[i]->Initialize(device, D3DUtils::GetRequiredConstantDataSize(CONST_TYPE_SIZE[i]), maxNumCbv);
	}
	return true;
}

void ConstantBufferManager::CleanUp()
{
	for (uint32 i = 0; i < static_cast<uint32>(CONSTANT_BUFFER_TYPE::CONST_TYPE_COUNT); i++)
	{
		delete m_constantBufferPool[i];
		m_constantBufferPool[i] = nullptr;
	}
}

void ConstantBufferManager::Free()
{
	for (uint32 i = 0; i < static_cast<uint32>(CONSTANT_BUFFER_TYPE::CONST_TYPE_COUNT); i++)
	{
		m_constantBufferPool[i]->Free();
	}
}

ConstantBufferPool* ConstantBufferManager::GetConstantBufferPool(CONSTANT_BUFFER_TYPE type)
{
	if (type >= CONSTANT_BUFFER_TYPE::CONST_TYPE_COUNT)
	{
		__debugbreak();
	}

	ConstantBufferPool* cbPool = m_constantBufferPool[static_cast<uint32>(type)];

	return cbPool;
}

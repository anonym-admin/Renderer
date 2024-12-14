#include "pch.h"
#include "TextureManager.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "DescriptorAllocator.h"

/*
=================
TextureManager
=================
*/

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
	CleanUp();
}

bool TextureManager::Initialize(Renderer* renderer)
{
    m_renderer = renderer;

    m_hashTable = HT_CreateHashTable(32);

    return true;
}

TEXTURE_HANDLE* TextureManager::CreateTiledTexture(uint32 texWidth, uint32 texHeight, uint32 cellWidth, uint32 cellHeight)
{
	ID3D12Device5* device = m_renderer->GetDevice();
	ID3D12Resource* texResource = nullptr;
	ResourceManager* resourceManager = m_renderer->GetReourceManager();
	DescriptorAllocator* descriptorAllocator = m_renderer->GetDescriptorAllocator();
	D3D12_RESOURCE_DESC texDesc = {};
	TEXTURE_HANDLE* textureHandle = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	uint8* image = (uint8*)malloc(texWidth * texHeight * 4);
	resourceManager->CreateTiledImage(image, texWidth, texHeight, cellWidth, cellHeight);
	
	resourceManager->CreateTextureWidthImageData(&texResource, image, &texDesc, texWidth, texHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	if (texResource)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texDesc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texDesc.MipLevels;

		descriptorAllocator->AllocateDescriptorHeap(&srv);
		if (srv.ptr)
		{
			device->CreateShaderResourceView(texResource, &srvDesc, srv);

			textureHandle = new TEXTURE_HANDLE;
			::memset(textureHandle, 0, sizeof(TEXTURE_HANDLE));
			textureHandle->textureResource = texResource;
			textureHandle->srv = srv;
		}
		else
		{
			texResource->Release();
			texResource = nullptr;
		}
	}

	delete[] image;
	image = nullptr;

	return textureHandle;
}

TEXTURE_HANDLE* TextureManager::CreateTextureFromFile(const wchar_t* filename)
{
	ID3D12Device5* device = m_renderer->GetDevice();
	ResourceManager* resourceManager = m_renderer->GetReourceManager();
	DescriptorAllocator* descriptorAllocator = m_renderer->GetDescriptorAllocator();
	TEXTURE_HANDLE* textureHandle = nullptr;
	ID3D12Resource* texResource = nullptr;
	D3D12_RESOURCE_DESC resDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	void* findValue = HT_Find(m_hashTable, (void*)filename);
	if (findValue)
	{
		textureHandle = reinterpret_cast<TEXTURE_HANDLE*>(findValue);
	}
	else
	{
		resourceManager->CreateTextureFromFile(&texResource, &resDesc, filename);
		if (texResource)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = resDesc.Format;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

			descriptorAllocator->AllocateDescriptorHeap(&srv);
			if (srv.ptr)
			{
				device->CreateShaderResourceView(texResource, &srvDesc, srv);

				textureHandle = new TEXTURE_HANDLE;
				::memset(textureHandle, 0, sizeof(TEXTURE_HANDLE));
				textureHandle->textureResource = texResource;
				textureHandle->srv = srv;

				HT_Insert(m_hashTable, (void*)filename, (void*)textureHandle);
			}
			else
			{
				texResource->Release();
				texResource = nullptr;
			}
		}
	}

	return textureHandle;
}

TEXTURE_HANDLE* TextureManager::CreateDynamicTexture(uint32 texWidth, uint32 texHeight, const char* name)
{
	ID3D12Device5* device = m_renderer->GetDevice();
	ResourceManager* resourceManager = m_renderer->GetReourceManager();
	DescriptorAllocator* descriptorAllocator = m_renderer->GetDescriptorAllocator();
	TEXTURE_HANDLE* textureHandle = nullptr;
	ID3D12Resource* texResource = nullptr;
	ID3D12Resource* uploadBuffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	resourceManager->CreateTextureWidthUploadBuffer(&texResource, &uploadBuffer, texWidth, texHeight, format);
	if (texResource && uploadBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		descriptorAllocator->AllocateDescriptorHeap(&srv);
		if (srv.ptr)
		{
			device->CreateShaderResourceView(texResource, &srvDesc, srv);

			textureHandle = new TEXTURE_HANDLE;
			::memset(textureHandle, 0, sizeof(TEXTURE_HANDLE));
			textureHandle->textureResource = texResource;
			textureHandle->uploadBuffer = uploadBuffer;
			textureHandle->srv = srv;
			strcpy_s(textureHandle->name, name);
		}
		else
		{
			texResource->Release();
			texResource = nullptr;

			uploadBuffer->Release();
			uploadBuffer = nullptr;
		}
	}

	return textureHandle;
}

void TextureManager::DestroyTexture(TEXTURE_HANDLE* textureHandle)
{
	DescriptorAllocator* descriptorAllocator = m_renderer->GetDescriptorAllocator();
	TEXTURE_HANDLE* texHandle = (TEXTURE_HANDLE*)textureHandle;

	if (texHandle)
	{
		if (texHandle->textureResource)
		{
			texHandle->textureResource->Release();
			texHandle->textureResource = nullptr;
		}
		if (texHandle->uploadBuffer)
		{
			texHandle->uploadBuffer->Release();
			texHandle->uploadBuffer = nullptr;
		}
		if (texHandle->srv.ptr)
		{
			descriptorAllocator->FreeDecriptorHeap(texHandle->srv);
		}

		delete texHandle;
	}
}

void TextureManager::CleanUp()
{
	for (uint32 i = 0; i < m_hashTable->tableSize; i++)
	{
		DL_LIST* cur = m_hashTable->headList[i];
		while (cur != nullptr)
		{
			DL_LIST* next = cur->next;
			Bucket* bucket = reinterpret_cast<Bucket*>(cur);
			TEXTURE_HANDLE* texHandle = reinterpret_cast<TEXTURE_HANDLE*>(bucket->value);

			DestroyTexture(texHandle);

			cur = next;
		}
	}

    if (m_hashTable)
    {
        HT_DestroyHashTable(m_hashTable);
    }
}

#pragma once

/*
===============
Renderer Type
===============
*/
#include "../../Common/Type.h"
#include <d3d12.h>
#include <dwrite.h>
#include <directxtk/SimpleMath.h>
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector2;

/*
========================
Constant Buffer Type
========================
*/

struct MESH_CONST_DATA
{
	Matrix world;
	Matrix view;
	Matrix projection;
};

struct SPRITE_CONST_DATA
{
	Vector2 screenResolution;
	Vector2 posOffset;
	Vector2 scale;
	Vector2 texSize;
	Vector2 texOffset;
	Vector2 texScale;
	float depthZ;
	float alpha;
};

enum class CONSTANT_BUFFER_TYPE
{
	MESH_CONST_TYPE,
	SPRITE_CONST_TYPE,
	CONST_TYPE_COUNT,
};

/*
===================
Command List
===================
*/

struct COMMAND_CONTEXT_HANDLE
{
	DL_LIST link;
	ID3D12GraphicsCommandList* cmdList = nullptr;
	ID3D12CommandAllocator* cmdAllocator = nullptr;
};

/*
============
Texture
============
*/

struct TEXTURE_HANDLE
{
	ID3D12Resource* textureResource = nullptr;
	ID3D12Resource* uploadBuffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	char name[32] = {};
};

/*
============
Mesh
============
*/

struct MESH
{
	ID3D12Resource* vertexBuffer = nullptr;
	ID3D12Resource* indexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	uint32 numIndices = 0;
	TEXTURE_HANDLE* textureHandle = nullptr;
};

/*
============
Font
============
*/

struct FONT_HANDLE
{
	wchar_t fontName[256] = {};
	float fontSize = 16.0f;
	IDWriteTextFormat* textFormat = nullptr;
};

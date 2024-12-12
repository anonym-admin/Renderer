#pragma once

/*
===============
Renderer Type
===============
*/
#include "../../Common/Type.h"
#include <d3d12.h>
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

};

enum class CONSTANT_BUFFER_TYPE
{
	MESH_CONST_TYPE,
	SPRITE_CONST_TYPE,
	CONST_TYPE_COUNT,
};

/*
============
Texture
============
*/

struct TEXTURE_HANDLE
{
	ID3D12Resource* textureResource;
	ID3D12Resource* uploadBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE srv;
};

/*
============
Mesh
============
*/

struct MESH
{
	ID3D12Resource* vertexBuffer;
	ID3D12Resource* indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	uint32 numIndices;
	TEXTURE_HANDLE* textureHandle;
};

/*
============
Font
============
*/

struct FONT_HANDLE
{
	wchar_t fontName[256];
	float fontSize;
	IDWriteTextFormat* textFormat;
};

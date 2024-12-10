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

struct MeshConstData
{
	Matrix world;
	Matrix view;
	Matrix projection;
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
};


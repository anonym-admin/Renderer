#include "pch.h"
#include "SpriteObject.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "ConstantBufferManager.h"
#include "ConstantBufferPool.h"
#include "ConstantBuffer.h"
#include "DescriptorPool.h"

/*
=================
SpriteObject
=================
*/

uint32 SpriteObject::sm_initRefCount;
ID3D12RootSignature* SpriteObject::sm_rootSignature;
ID3D12PipelineState* SpriteObject::sm_pipelineState;
D3D12_VERTEX_BUFFER_VIEW SpriteObject::sm_vbView;
D3D12_INDEX_BUFFER_VIEW SpriteObject::sm_ibView;
ID3D12Resource* SpriteObject::sm_vertexBuffer;
ID3D12Resource* SpriteObject::sm_indexBuffer;

SpriteObject::SpriteObject()
{
}

SpriteObject::~SpriteObject()
{
	CleanUp();
}

bool SpriteObject::Initialize(Renderer* renderer)
{
	m_renderer = renderer;

	bool result = true;
	if (sm_initRefCount == 0)
	{
		result = InitPipeline();
	}

	sm_initRefCount++;

	return result;
}

bool SpriteObject::Initialize(Renderer* renderer, const wchar_t* filename, const RECT* rect)
{
	m_renderer = renderer;

	bool result = true;
	if (sm_initRefCount == 0)
	{
		result = InitPipeline();
	}
	
	uint32 texWidth = 1;
	uint32 texHeight = 1;
	m_textureHandle = reinterpret_cast<TEXTURE_HANDLE*>(m_renderer->CreateTextureFromFile(filename));
	if (m_textureHandle)
	{
		D3D12_RESOURCE_DESC desc = m_textureHandle->textureResource->GetDesc();
		texWidth = static_cast<uint32>(desc.Width);
		texHeight = static_cast<uint32>(desc.Height);
	}
	if (rect)
	{
		m_rect = *rect;
		m_scale.x = static_cast<float>(rect->right - rect->left) / texWidth;
		m_scale.y = static_cast<float>(rect->bottom - rect->top) / texHeight;
	}
	else
	{
		if (m_textureHandle)
		{
			m_rect.left = 0;
			m_rect.top = 0;
			m_rect.right = texWidth;
			m_rect.bottom = texHeight;
		}
	}

	sm_initRefCount++;

	return true;
}

void SpriteObject::Draw(ID3D12GraphicsCommandList* cmdList, uint32 threadIdx, float posX, float posY, float scaleX, float scaleY, float z)
{
	Vector2 scale = Vector2(m_scale.x * scaleX, m_scale.y * scaleY);
	DrawWithTexture(cmdList, threadIdx, posX, posY, scale.x, scale.y, z, &m_rect, m_textureHandle);
}

void SpriteObject::DrawWithTexture(ID3D12GraphicsCommandList* cmdList, uint32 threadIdx, float posX, float posY, float scaleX, float scaleY, float z, const RECT* rect, TEXTURE_HANDLE* textureHandle)
{
	ID3D12Device5* device = m_renderer->GetDevice();
	ConstantBufferManager* cbManager = m_renderer->GetConstantBufferManager(threadIdx);
	ConstantBufferPool* cbPool = cbManager->GetConstantBufferPool(CONSTANT_BUFFER_TYPE::SPRITE_CONST_TYPE);
	DescriptorPool* descPool = m_renderer->GetDescriptorPool(threadIdx);
	ID3D12DescriptorHeap* descHeap = descPool->GetDesciptorHeap();

	uint32 texWidth = 0;
	uint32 texHeight = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	if (textureHandle)
	{
		D3D12_RESOURCE_DESC desc = textureHandle->textureResource->GetDesc();
		texWidth = static_cast<uint32>(desc.Width);
		texHeight = static_cast<uint32>(desc.Height);
		srv = textureHandle->srv;
	}

	RECT rt = {};
	if (!rect)
	{
		rt.left = 0;
		rt.top = 0;
		rt.right = texWidth;
		rt.bottom = texHeight;
		rect = &rt;
	}

	ConstantBuffer* constantBuffer = cbPool->Alloc();
	SPRITE_CONST_DATA constData = {};

	if (constantBuffer)
	{
		constData.screenResolution.x = static_cast<float>(m_renderer->GetScreenWidth());
		constData.screenResolution.y = static_cast<float>(m_renderer->GetScreenHegiht());
		constData.posOffset.x = posX;
		constData.posOffset.y = posY;
		constData.scale.x = scaleX;
		constData.scale.y = scaleY;
		constData.texSize.x = static_cast<float>(texWidth);
		constData.texSize.y = static_cast<float>(texHeight);
		constData.texOffset.x = static_cast<float>(rect->left);
		constData.texOffset.y = static_cast<float>(rect->top);
		constData.texScale.x = static_cast<float>(rect->right - rect->left);
		constData.texScale.y = static_cast<float>(rect->bottom - rect->top);
		constData.depthZ = z;
		constData.alpha = 1.0f;

		constantBuffer->Upload(&constData);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
	descPool->Alloc(&cpuHandle, &gpuHandle, MAX_DESCRIPTOR_COUNT_FOR_DRAW);
	
	cmdList->SetGraphicsRootSignature(sm_rootSignature);
	cmdList->SetPipelineState(sm_pipelineState);
	cmdList->SetDescriptorHeaps(1, &descHeap);

	device->CopyDescriptorsSimple(1, cpuHandle, constantBuffer->GetCbvHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.Offset(1, descPool->GetTypeSize());
	if (srv.ptr)
	{
		device->CopyDescriptorsSimple(1, cpuHandle, srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	cmdList->SetGraphicsRootDescriptorTable(0, gpuHandle);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &sm_vbView);
	cmdList->IASetIndexBuffer(&sm_ibView);
	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

HRESULT __stdcall SpriteObject::QueryInterface(REFIID riid, void** ppvObject)
{
	return E_NOTIMPL;
}

ULONG __stdcall SpriteObject::AddRef(void)
{
	uint32 refCount = ++m_refCount;
	return refCount;
}

ULONG __stdcall SpriteObject::Release(void)
{
	uint32 refCount = --m_refCount;
	if (refCount == 0)
	{
		delete this;
	}
	return refCount;
}

void SpriteObject::CleanUp()
{
	m_renderer->GpuCompleted();

	uint32 refCount = --sm_initRefCount;
	if (refCount == 0)
	{
		CleanUpPipeline();
	}
}

bool SpriteObject::InitPipeline()
{
	CreateRootSignature();
	CreatePipelineState();
	CreateBuffers();
	return true;
}

void SpriteObject::CleanUpPipeline()
{
	DestroyBuffers();
	DestroyPipelineState();
	DestroyRootSignature();
}

void SpriteObject::CreateRootSignature()
{
	ID3D12Device5* device = m_renderer->GetDevice();

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	CD3DX12_DESCRIPTOR_RANGE rangesPerObj[2] = {};
	rangesPerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0 
	rangesPerObj[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

	CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangesPerObj), rangesPerObj, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_STATIC_SAMPLER_DESC samplterDesc = {};
	//pOutSamperDesc->Filter = D3D12_FILTER_ANISOTROPIC;
	samplterDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplterDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplterDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplterDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplterDesc.MipLODBias = 0.0f;
	samplterDesc.MaxAnisotropy = 16;
	samplterDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplterDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	samplterDesc.MinLOD = -FLT_MAX;
	samplterDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplterDesc.ShaderRegister = 0;
	samplterDesc.RegisterSpace = 0;
	samplterDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplterDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	D3DUtils::PrintError(error);

	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&sm_rootSignature)));

	if (signature)
	{
		signature->Release();
		signature = nullptr;
	}
	if (error)
	{
		error->Release();
		error = nullptr;
	}
}

void SpriteObject::CreatePipelineState()
{
	ID3D12Device5* device = m_renderer->GetDevice();

	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;
	ID3DBlob* error = nullptr;

	uint32 compileFlags = 0;
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	if (FAILED(D3DCompileFromFile(L"../../Shader/SpriteShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
	{
		if (error != nullptr)
		{
			D3DUtils::PrintError(error);
		}
		__debugbreak();
	}

	if (FAILED(D3DCompileFromFile(L"../../Shader/SpriteShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error)))
	{
		if (error != nullptr)
		{
			D3DUtils::PrintError(error);
		}
		__debugbreak();
	}

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = sm_rootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&sm_pipelineState)));

	if (vertexShader)
	{
		vertexShader->Release();
		vertexShader = nullptr;
	}
	if (pixelShader)
	{
		pixelShader->Release();
		pixelShader = nullptr;
	}
	if (error)
	{
		error->Release();
		error = nullptr;
	}
}

void SpriteObject::CreateBuffers()
{
	ResourceManager* resourceManager = m_renderer->GetReourceManager();
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ID3D12Resource* vertexBuffer = nullptr;
	ID3D12Resource* indexBuffer = nullptr;

	const uint32 numVertices = 4;
	const uint32 numIndices = 6;
	SpriteVertex vertices[numVertices] =
	{
		{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
		{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
	};
	uint32 indices[numIndices] =
	{
		0, 2, 1, 0, 3, 2
	};

	resourceManager->CreateVertexBuffer(sizeof(SpriteVertex), numVertices, vertices, &vbView, &vertexBuffer);
	resourceManager->CreateIndexBuffer(sizeof(uint32), numIndices, indices, &ibView, &indexBuffer);

	sm_vbView = vbView;
	sm_ibView = ibView;
	sm_vertexBuffer = vertexBuffer;
	sm_indexBuffer = indexBuffer;
}

void SpriteObject::DestroyRootSignature()
{
	if (sm_rootSignature)
	{
		sm_rootSignature->Release();
		sm_rootSignature = nullptr;
	}
}

void SpriteObject::DestroyPipelineState()
{
	if (sm_pipelineState)
	{
		sm_pipelineState->Release();
		sm_pipelineState = nullptr;
	}
}

void SpriteObject::DestroyBuffers()
{
	if (sm_indexBuffer)
	{
		sm_indexBuffer->Release();
		sm_indexBuffer = nullptr;
	}
	if (sm_vertexBuffer)
	{
		sm_vertexBuffer->Release();
		sm_vertexBuffer = nullptr;
	}
}

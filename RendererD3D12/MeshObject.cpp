#include "pch.h"
#include "MeshObject.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "ConstantBuffer.h"

/*
================
Mesh Object
================
*/

uint32 MeshObject::sm_initRefCount;
ID3D12RootSignature* MeshObject::sm_rootSignature;
ID3D12PipelineState* MeshObject::sm_pipelineState;

MeshObject::MeshObject()
{
	m_refCount++;
}

MeshObject::~MeshObject()
{
	CleanUp();
}

bool MeshObject::Initialize(Renderer* renderer)
{
	m_renderer = renderer;

	ID3D12Device5* device = m_renderer->GetDevice();

	bool result = true;
	if (sm_initRefCount == 0)
	{
		result = InitPipeline();
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvHeap));

	m_constantBuffer = new ConstantBuffer;
	m_constantBuffer->Initialize(m_renderer->GetDevice(), sizeof(MESH_CONST_DATA), m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

	sm_initRefCount++;
	return result;
}

void MeshObject::Draw(ID3D12GraphicsCommandList* cmdList, Matrix worldRow)
{
	Matrix viewMat, projMat;
	m_renderer->GetViewProjMatrix(&viewMat, &projMat);
	
	m_constData.world = worldRow.Transpose();
	m_constData.view = viewMat;
	m_constData.projection = projMat;

	m_constantBuffer->Upload(&m_constData);

	cmdList->SetGraphicsRootSignature(sm_rootSignature);
	cmdList->SetDescriptorHeaps(1, &m_cbvHeap);
	cmdList->SetPipelineState(sm_pipelineState);
	cmdList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

	for (uint32 i = 0; i < m_numMeshes; i++)
	{
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &m_meshes[i].vertexBufferView);
		cmdList->IASetIndexBuffer(&m_meshes[i].indexBufferView);
		cmdList->DrawIndexedInstanced(m_meshes[i].numIndices, 1, 0, 0, 0);
	}
}

void MeshObject::CreateMeshBuffers(MESH_GROUP_HANDLE* mgHandle)
{
	ResourceManager* resoureManager = m_renderer->GetReourceManager();

	ID3D12Resource* vertexBuffer = nullptr;
	ID3D12Resource* indexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};

	MeshData* meshDatas = mgHandle->meshes;
	uint32 numMeshes = mgHandle->numMeshes;

	m_meshes = new MESH[numMeshes];
	MESH* meshes = new MESH[numMeshes];

	for (uint32 i = 0; i < numMeshes; i++)
	{
		resoureManager->CreateVertexBuffer(sizeof(Vertex), meshDatas[i].numVertices, meshDatas[i].vertices, &vertexBufferView, &vertexBuffer);
		m_meshes[i].vertexBuffer = vertexBuffer;
		m_meshes[i].vertexBufferView = vertexBufferView;

		resoureManager->CreateIndexBuffer(sizeof(uint32), meshDatas[i].numIndices, meshDatas[i].indices, &indexBufferView, &indexBuffer);
		m_meshes[i].indexBuffer = indexBuffer;
		m_meshes[i].indexBufferView = indexBufferView;
		m_meshes[i].numIndices = meshDatas[i].numIndices;
	}
	
	m_numMeshes = numMeshes;

	//MESH* dest = m_meshes;
	//MESH* src = meshes;
	//memcpy(dest, src, sizeof(MESH) * numMeshes);

	delete[] meshes;
	meshes = nullptr;
}

void MeshObject::SetTransform(Matrix worldRow)
{
	m_constData.world = worldRow.Transpose();
}

HRESULT __stdcall MeshObject::QueryInterface(REFIID riid, void** ppvObject)
{
	return E_NOTIMPL;
}

ULONG __stdcall MeshObject::AddRef(void)
{
	uint32 refCount = ++m_refCount;
	return refCount;
}

ULONG __stdcall MeshObject::Release(void)
{
	uint32 refCount = --m_refCount;
	if (refCount == 0)
	{
		delete this;
	}
	return refCount;
}

void MeshObject::CleanUp()
{
	m_renderer->GpuCompleted();

	if (m_cbvHeap)
	{
		m_cbvHeap->Release();
		m_cbvHeap = nullptr;
	}

	if (m_constantBuffer)
	{
		m_constantBuffer->CleanUp();
		delete m_constantBuffer;
		m_constantBuffer = nullptr;
	}

	if (m_meshes)
	{
		for (uint32 i = 0; i < m_numMeshes; i++)
		{
			if (m_meshes[i].vertexBuffer)
			{
				m_meshes[i].vertexBuffer->Release();
				m_meshes[i].vertexBuffer = nullptr;
			}
			if (m_meshes[i].indexBuffer)
			{
				m_meshes[i].indexBuffer->Release();
				m_meshes[i].indexBuffer = nullptr;
			}
		}

		delete[] m_meshes;
		m_meshes = nullptr;
	}


	uint32 refCount = --sm_initRefCount;
	if (refCount == 0)
	{
		CleanUpPipeline();
	}
}

bool MeshObject::InitPipeline()
{
	CreateRootSignature();
	CreatePipelineState();
	return true;
}

void MeshObject::CleanUpPipeline()
{
	DestroyPipelineState();
	DestroyRootSignature();
}

void MeshObject::CreateRootSignature()
{
	ID3D12Device5* device = m_renderer->GetDevice();

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	CD3DX12_DESCRIPTOR_RANGE rangesPerObj[1] = {};
	rangesPerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0 

	CD3DX12_DESCRIPTOR_RANGE rangesPerTriGroup[1] = {};
	rangesPerTriGroup[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangesPerObj), rangesPerObj, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(_countof(rangesPerTriGroup), rangesPerTriGroup, D3D12_SHADER_VISIBILITY_ALL);

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

void MeshObject::CreatePipelineState()
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

	if (FAILED(D3DCompileFromFile(L"../../Shader/BasicShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
	{
		if (error != nullptr)
		{
			D3DUtils::PrintError(error);
		}
		__debugbreak();
	}

	if (FAILED(D3DCompileFromFile(L"../../Shader/BasicShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error)))
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
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

void MeshObject::DestroyRootSignature()
{
	if (sm_rootSignature)
	{
		sm_rootSignature->Release();
		sm_rootSignature = nullptr;
	}
}

void MeshObject::DestroyPipelineState()
{
	if (sm_pipelineState)
	{
		sm_pipelineState->Release();
		sm_pipelineState = nullptr;
	}
}

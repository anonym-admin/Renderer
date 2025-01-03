#include "pch.h"
#include "LineObject.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "DescriptorPool.h"
#include "ConstantBufferPool.h"
#include "ConstantBufferManager.h"

/*
================
LineObject
================
*/

uint32 LineObject::sm_initRefCount;
ID3D12RootSignature* LineObject::sm_rootSignature;
ID3D12PipelineState* LineObject::sm_pipelineState;

LineObject::LineObject()
{
	m_refCount++;
}

LineObject::~LineObject()
{
	CleanUp();
}

bool LineObject::Initialize(Renderer* renderer)
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

void LineObject::Draw(ID3D12GraphicsCommandList* cmdList, uint32 threadIdx, Matrix worldRow)
{
	ID3D12Device5* device = m_renderer->GetDevice();
	ConstantBufferManager* cbManager = m_renderer->GetConstantBufferManager(threadIdx);
	ConstantBufferPool* cbPool = cbManager->GetConstantBufferPool(CONSTANT_BUFFER_TYPE::MESH_CONST_TYPE);
	DescriptorPool* descPool = m_renderer->GetDescriptorPool(threadIdx);
	ID3D12DescriptorHeap* descHeap = descPool->GetDesciptorHeap();

	MESH_CONST_DATA constData = {};
	ConstantBuffer* constantBuffer = cbPool->Alloc();
	if (constantBuffer)
	{
		Matrix viewMat, projMat;
		m_renderer->GetViewProjMatrix(&viewMat, &projMat);

		constData.world = worldRow.Transpose();
		constData.view = viewMat;
		constData.projection = projMat;

		memcpy(constantBuffer->sysMemAddr, &constData, sizeof(MESH_CONST_DATA));
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
	descPool->Alloc(&cpuHandle, &gpuHandle, DESCRIPTOR_COUNT_PER_OBJ);

	cmdList->SetGraphicsRootSignature(sm_rootSignature);
	cmdList->SetPipelineState(sm_pipelineState);
	cmdList->SetDescriptorHeaps(1, &descHeap);

	device->CopyDescriptorsSimple(1, cpuHandle, constantBuffer->cbvCpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	cmdList->SetGraphicsRootDescriptorTable(0, gpuHandle);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	cmdList->IASetVertexBuffers(0, 1, &m_vbView);
	cmdList->DrawInstanced(m_numVertices, 1, 0, 0);
}

void LineObject::CreateLineBuffers(LineData* lineData)
{
	ResourceManager* resourceManager = m_renderer->GetReourceManager();
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ID3D12Resource* vertexBuffer = nullptr;
	ID3D12Resource* indexBuffer = nullptr;

	m_numVertices = lineData->numVertices;
	LineVertex* vertices = lineData->vertices;

	resourceManager->CreateVertexBuffer(sizeof(LineVertex), m_numVertices, vertices, &vbView, &vertexBuffer);

	m_vbView = vbView;
	m_vertexBuffer = vertexBuffer;
}

HRESULT __stdcall LineObject::QueryInterface(REFIID riid, void** ppvObject)
{
	return E_NOTIMPL;
}

ULONG __stdcall LineObject::AddRef(void)
{
	uint32 refCount = ++m_refCount;
	return refCount;
}

ULONG __stdcall LineObject::Release(void)
{
	uint32 refCount = --m_refCount;
	if (refCount == 0)
	{
		delete this;
	}
	return refCount;
}

void LineObject::CleanUp()
{
	m_renderer->GpuCompleted();

	uint32 refCount = --sm_initRefCount;
	if (refCount == 0)
	{
		CleanUpPipeline();
	}
}

bool LineObject::InitPipeline()
{
	CreateRootSignature();
	CreatePipelineState();
	return true;
}

void LineObject::CleanUpPipeline()
{
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}

	DestroyPipelineState();
	DestroyRootSignature();
}

void LineObject::CreateRootSignature()
{
	ID3D12Device5* device = m_renderer->GetDevice();

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	CD3DX12_DESCRIPTOR_RANGE rangesPerObj[1] = {};
	rangesPerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0 

	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
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

void LineObject::CreatePipelineState()
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

	if (FAILED(D3DCompileFromFile(L"../../Shader/LineShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
	{
		if (error != nullptr)
		{
			D3DUtils::PrintError(error);
		}
		__debugbreak();
	}

	if (FAILED(D3DCompileFromFile(L"../../Shader/LineShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error)))
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
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
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
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
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


void LineObject::DestroyRootSignature()
{
	if (sm_rootSignature)
	{
		sm_rootSignature->Release();
		sm_rootSignature = nullptr;
	}
}

void LineObject::DestroyPipelineState()
{
	if (sm_pipelineState)
	{
		sm_pipelineState->Release();
		sm_pipelineState = nullptr;
	}
}


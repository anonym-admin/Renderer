#pragma once

#include "../../Interface/IT_Renderer.h"

/*
================
Mesh Object
================
*/

class Renderer;
class ConstantBuffer;

class MeshObject : public IT_MeshObject
{
public:
	MeshObject();
	~MeshObject();

	/*Dll Inner*/
	bool Initialize(Renderer* renderer);
	void Draw(ID3D12GraphicsCommandList* cmdList, Matrix worldRow);

	/*Interface*/
	virtual void CreateMeshBuffers(MESH_GROUP_HANDLE* mgHandle);
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

private:
	void CleanUp();
	bool InitPipeline();
	void CleanUpPipeline();
	void CreateRootSignature();
	void CreatePipelineState();
	void DestroyRootSignature();
	void DestroyPipelineState();

private:
	static uint32 sm_initRefCount;
	static ID3D12RootSignature* sm_rootSignature;
	static ID3D12PipelineState* sm_pipelineState;
	uint32 m_refCount = 0;
	Renderer* m_renderer = nullptr;
	MESH* m_meshes = nullptr;
	uint32 m_numMeshes = 0;
	ID3D12DescriptorHeap* m_cbvHeap = nullptr;
	MeshConstData m_constData = {};
	ConstantBuffer* m_constantBuffer = nullptr;
};


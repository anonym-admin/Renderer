#pragma once

#include "../../Interface/IT_Renderer.h"

/*
================
Mesh Object
================
*/

class Renderer;

class MeshObject : public IT_MeshObject
{
public:
	static const uint32 DESCRIPTOR_COUNT_PER_OBJ = 1;		// CB(b0)
	static const uint32 DESCRIPTOR_COUNT_PER_MESH_DATA = 1; // SRV(t0)
	static const uint32 MAX_MESH_DATA_COUNT_PER_OBJ = 8;
	static const uint32 MAX_DESCRIPTOR_COUNT_FOR_DRAW = DESCRIPTOR_COUNT_PER_OBJ + (DESCRIPTOR_COUNT_PER_MESH_DATA * MAX_MESH_DATA_COUNT_PER_OBJ);

	MeshObject();
	~MeshObject();

	/*DLL Inner*/
	bool Initialize(Renderer* renderer);
	void Draw(ID3D12GraphicsCommandList* cmdList, uint32 threadIdx, Matrix worldRow, bool isWire = false);

	/*Interface*/
	virtual void CreateMeshBuffers(const MeshData* meshData, const uint32 numMeshes) override;
	virtual void SetTexture(void* textureHandle) override;
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
	static ID3D12PipelineState* sm_defaultPSO;
	static ID3D12PipelineState* sm_wirePSO;
	Renderer* m_renderer = nullptr;
	MESH* m_meshes = nullptr;
	uint32 m_refCount = 0;
	uint32 m_numMeshes = 0;
};


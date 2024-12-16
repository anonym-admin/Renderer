#pragma once

#include "../../Interface/IT_Renderer.h"

/*
================
LineObject
================
*/

class Renderer;
class ConstantBuffer;

class LineObject : public IT_LineObject
{
public:
	static const uint32 DESCRIPTOR_COUNT_PER_OBJ = 1;		// CB(b0)
	// static const uint32 DESCRIPTOR_COUNT_PER_MESH_DATA = 1; // SRV(t0)

	LineObject();
	~LineObject();

	/*DLL Inner*/
	bool Initialize(Renderer* renderer);
	void Draw(ID3D12GraphicsCommandList* cmdList, uint32 threadIdx, Matrix worldRow);

	/*Interface*/
	virtual void CreateLineBuffers(LineData* lineData) override;
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
	D3D12_VERTEX_BUFFER_VIEW m_vbView = {};
	ID3D12Resource* m_vertexBuffer = nullptr;
	MESH_CONST_DATA m_constData = {};
	Renderer* m_renderer = nullptr;
	uint32 m_refCount = 0;
	uint32 m_numVertices = 0;
};


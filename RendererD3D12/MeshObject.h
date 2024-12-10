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
	MeshObject();
	~MeshObject();

	/*Dll Inner*/
	bool Initialize(Renderer* renderer);

	/*Interface*/
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
	static uint32 sm_refCount;
	Renderer* m_renderer = nullptr;
	ID3D12RootSignature* m_rootSignature = nullptr;
	ID3D12PipelineState* m_pipelineState = nullptr;
};


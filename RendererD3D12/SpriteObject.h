#pragma once

#include "../../Interface/IT_Renderer.h"

/*
=================
SpriteObject
=================
*/

class Renderer;

class SpriteObject : public IT_SpriteObject
{
public:
	static const uint32 MAX_DESCRIPTOR_COUNT_FOR_DRAW = 2;

	SpriteObject();
	~SpriteObject();

	/*DLL Inner*/
	bool Initialize(Renderer* renderer);
	bool Initialize(Renderer* renderer, const wchar_t* filename, const RECT* rect);
	void Draw(ID3D12GraphicsCommandList* cmdList, uint32 threadIdx, float posX, float posY, float scaleX, float scaleY, float z);
	void DrawWithTexture(ID3D12GraphicsCommandList* cmdList, uint32 threadIdx, float posX, float posY, float scaleX, float scaleY, float z, const RECT* rect, TEXTURE_HANDLE* textureHandle);

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
	void CreateBuffers();
	void DestroyRootSignature();
	void DestroyPipelineState();
	void DestroyBuffers();

private:
	static uint32 sm_initRefCount;
	static ID3D12RootSignature* sm_rootSignature;
	static ID3D12PipelineState* sm_pipelineState;
	static D3D12_VERTEX_BUFFER_VIEW sm_vbView;
	static D3D12_INDEX_BUFFER_VIEW sm_ibView;
	static ID3D12Resource* sm_vertexBuffer;
	static ID3D12Resource* sm_indexBuffer;
	Renderer* m_renderer = nullptr;
	uint32 m_refCount = 1;
	SPRITE_CONST_DATA m_constData = {};
	TEXTURE_HANDLE* m_textureHandle = nullptr;
	RECT m_rect = {};
	Vector2 m_scale = Vector2(1.0f);
};


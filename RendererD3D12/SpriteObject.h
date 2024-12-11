#pragma once

#include "../../Interface/IT_Renderer.h"

/*
=================
SpriteObject
=================
*/

class SpriteObject : public IT_Renderer
{
public:
	static const uint32 MAX_DESCRIPTOR_COUNT_FOR_DRAW = 2;

	SpriteObject();
	~SpriteObject();
	
	bool Initialize();

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
};


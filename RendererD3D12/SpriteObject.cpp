#include "pch.h"
#include "SpriteObject.h"

/*
=================
SpriteObject
=================
*/

uint32 SpriteObject::sm_initRefCount;
ID3D12RootSignature* SpriteObject::sm_rootSignature;
ID3D12PipelineState* SpriteObject::sm_pipelineState;

SpriteObject::SpriteObject()
{
}

SpriteObject::~SpriteObject()
{
	CleanUp();
}

bool SpriteObject::Initialize()
{
	return true;
}

void SpriteObject::CleanUp()
{
}

bool SpriteObject::InitPipeline()
{
	return true;
}

void SpriteObject::CleanUpPipeline()
{
}

void SpriteObject::CreateRootSignature()
{
}

void SpriteObject::CreatePipelineState()
{
}

void SpriteObject::DestroyRootSignature()
{
}

void SpriteObject::DestroyPipelineState()
{
}

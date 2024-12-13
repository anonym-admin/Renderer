#pragma once

/*
================
RenderQueue
================
*/

enum class RENDER_JOB_TYPE
{
	RENDER_MESH_OBJECT,
	RENDER_SPRITE_OBJECT,
};

struct MESH_RENDER_JOB
{
	Matrix worldRow;
};

struct SPRITE_RENDER_JOB
{
	uint32 posX;
	uint32 posY;
	float scaleX;
	float scaleY;
	float z;
	const RECT* rect;
	void* texHandle;
};

struct RENDER_JOB
{
	RENDER_JOB_TYPE type = {};
	void* obj = nullptr;
	union
	{
		MESH_RENDER_JOB mesh;
		SPRITE_RENDER_JOB sprite;
	};
};

class RenderQueue
{
public:
	RenderQueue();
	~RenderQueue();

	bool Initialize(ID3D12Device5* device, uint32 maxNumJob);
	void Add(const RENDER_JOB* renderJob);
	void Free();
	uint32 Process(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewPort, D3D12_RECT scissorRect);

private:
	void CleanUp();
	const RENDER_JOB* Dispatch();

private:
	ID3D12Device5* m_device = nullptr;
	uint8* m_queueBuffer = nullptr;
	uint32 m_maxBufferSize = 0;
	uint32 m_readPos = 0;
	uint32 m_writePos = 0;
};


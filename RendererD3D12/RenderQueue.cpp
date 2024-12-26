#include "pch.h"
#include "RenderQueue.h"
#include "MeshObject.h"
#include "SpriteObject.h"
#include "LineObject.h"
#include "CommandContext.h"

/*
================
RenderQueue
================
*/

RenderQueue::RenderQueue()
{
}

RenderQueue::~RenderQueue()
{
	CleanUp();
}

bool RenderQueue::Initialize(ID3D12Device5* device, uint32 maxNumJob)
{
	m_device = device;
	m_maxBufferSize = sizeof(RENDER_JOB) * maxNumJob;
	m_queueBuffer = (uint8*)malloc(m_maxBufferSize);

	m_readPos = 0;
	m_writePos = 0;

	return true;
}

void RenderQueue::Add(const RENDER_JOB* renderJob)
{
	if (m_writePos >= m_maxBufferSize)
	{
		__debugbreak();
	}

	uint8* dest = m_queueBuffer + m_writePos;
	const RENDER_JOB* src = renderJob;
	memcpy(dest, src, sizeof(RENDER_JOB));

	m_writePos += sizeof(RENDER_JOB);
	m_jobCount++;
}

void RenderQueue::Free()
{
	m_writePos = 0;
	m_readPos = 0;
}

uint32 RenderQueue::Process(uint32 threadIdx, CommandContext* cmdCtx, ID3D12CommandQueue* cmdQueue, uint32 processCountPerCmdList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewPort, D3D12_RECT scissorRect)
{
	const RENDER_JOB* job = nullptr;
	ID3D12GraphicsCommandList* cmdList = nullptr;
	ID3D12GraphicsCommandList* cmdLists[64] = {};
	uint32 processCount = 0;
	uint32 procCountPerCmdList = 0;
	uint32 cmdListCount = 0;

	while (job = Dispatch())
	{
		cmdList = cmdCtx->GetCurrentCommandList();
		cmdList->RSSetViewports(1, &viewPort);
		cmdList->RSSetScissorRects(1, &scissorRect);
		cmdList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		switch (job->type)
		{
			case RENDER_JOB_TYPE::RENDER_MESH_OBJECT:
			{
				MeshObject* meshObj = reinterpret_cast<MeshObject*>(job->obj);
				if (!meshObj)
				{
					__debugbreak();
				}

				printf("%lf ", job->mesh.worldRow._41);

				meshObj->Draw(cmdList, threadIdx, job->mesh.worldRow, job->mesh.isWire);
			}
			break;
			case RENDER_JOB_TYPE::RENDER_SPRITE_OBJECT:
			{
				SpriteObject* spriteObj = reinterpret_cast<SpriteObject*>(job->obj);
				if (!spriteObj)
				{
					__debugbreak();
				}
				SPRITE_RENDER_JOB param = {};
				param.posX = job->sprite.posX;
				param.posY = job->sprite.posY;
				param.scaleX = job->sprite.scaleX;
				param.scaleY = job->sprite.scaleY;
				param.z = job->sprite.z;
				param.rect = job->sprite.rect;
				param.texHandle = job->sprite.texHandle;

				TEXTURE_HANDLE* texHandle = reinterpret_cast<TEXTURE_HANDLE*>(param.texHandle);

				// printf("%d %d %s\n", param.posX, param.posY, texHandle->name);


				if (texHandle)
				{
					if (texHandle->uploadBuffer)
					{
						D3DUtils::UpdateTexture(m_device, cmdList, texHandle->textureResource, texHandle->uploadBuffer);
					}

					spriteObj->DrawWithTexture(cmdList, threadIdx, static_cast<float>(param.posX), static_cast<float>(param.posY), param.scaleX, param.scaleY, param.z, param.rect, texHandle);
				}
				else
				{
					spriteObj->Draw(cmdList, threadIdx, static_cast<float>(param.posX), static_cast<float>(param.posY), param.scaleX, param.scaleY, param.z);
				}
			}
			break;
			case RENDER_JOB_TYPE::RENDER_LINE_OBJECT:
			{
				LineObject* lineObj = reinterpret_cast<LineObject*>(job->obj);
				if (!lineObj)
				{
					__debugbreak();
				}
				lineObj->Draw(cmdList, threadIdx, job->line.worldRow);
			}
			break;
			default:
			{
				__debugbreak();
			}
			break;
		}

		processCount++;
		procCountPerCmdList++;

		if (procCountPerCmdList > processCountPerCmdList)
		{
			cmdCtx->Close();
			cmdLists[cmdListCount] = cmdList;
			cmdListCount++;
			cmdList = nullptr;
			procCountPerCmdList = 0;
		}
	}

	if (procCountPerCmdList)
	{
		cmdCtx->Close();
		cmdLists[cmdListCount] = cmdList;
		cmdList = nullptr;
		cmdListCount++;
		procCountPerCmdList = 0;
	}

	if (cmdListCount)
	{
		cmdQueue->ExecuteCommandLists(cmdListCount, (ID3D12CommandList**)cmdLists);
	}
	
	m_jobCount = 0;

	return processCount;
}

void RenderQueue::CleanUp()
{
	if (m_queueBuffer)
	{
		free(m_queueBuffer);
		m_queueBuffer = nullptr;
	}
}

const RENDER_JOB* RenderQueue::Dispatch()
{
	RENDER_JOB* job = nullptr;

	if (m_readPos >= m_writePos)
	{
		return job;
	}

	job = reinterpret_cast<RENDER_JOB*>(m_queueBuffer + m_readPos);
	
	m_readPos += sizeof(RENDER_JOB);

	return job;
}

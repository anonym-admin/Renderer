#include "pch.h"
#include "RenderQueue.h"
#include "MeshObject.h"
#include "SpriteObject.h"

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
}

void RenderQueue::Free()
{
	m_writePos = 0;
	m_readPos = 0;
}

uint32 RenderQueue::Process(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewPort, D3D12_RECT scissorRect)
{
	const RENDER_JOB* job = nullptr;

	while (job = Dispatch())
	{
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
			meshObj->Draw(cmdList, job->mesh.worldRow);
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
			if (texHandle)
			{
				if (texHandle->uploadBuffer)
				{
					D3DUtils::UpdateTexture(m_device, cmdList, texHandle->textureResource, texHandle->uploadBuffer);
				}

				spriteObj->DrawWithTexture(cmdList, static_cast<float>(param.posX), static_cast<float>(param.posY), param.scaleX, param.scaleY, param.z, param.rect, texHandle);
			}
			else
			{
				spriteObj->Draw(cmdList, static_cast<float>(param.posX), static_cast<float>(param.posY), param.scaleX, param.scaleY, param.z);
			}
		}
		break;
		default:
			break;
		}
	}
	return 0;
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

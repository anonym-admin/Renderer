#include "pch.h"
#include "RenderThread.h"
#include "Renderer.h"

/*
=====================
Render Thread
=====================
*/

uint32 RenderThread::ProcessByRenderThread(void* param)
{
	RENDER_THREAD_DESC* desc = reinterpret_cast<RENDER_THREAD_DESC*>(param);
	Renderer* renderer = reinterpret_cast<Renderer*>(desc->renderBody);
	uint32 threadIdx = desc->threadIdx;
	HANDLE* threadEvent = desc->threadEvent;
	bool exitFlag = false;
	
	while (true)
	{
		uint32 eventTypeIdx = WaitForMultipleObjects(static_cast<uint32>(RENDER_THREAD_EVENT_TYPE::RENDER_THREAD_TYPE_COUNT), threadEvent, false, INFINITE);
		RENDER_THREAD_EVENT_TYPE eventType = static_cast<RENDER_THREAD_EVENT_TYPE>(eventTypeIdx);

		switch (eventType)
		{
			case RENDER_THREAD_EVENT_TYPE::RENDER_THREAD_PROCESS:
				renderer->Process(threadIdx);
				break;
			case RENDER_THREAD_EVENT_TYPE::RENDER_THREAD_EXIT:
				exitFlag = true;
				break;
		}

		if (exitFlag)
		{
			// Exit render thread.
			break;
		}
	}

	_endthreadex(997);
	return 996;
}

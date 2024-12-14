#pragma once

/*
=====================
Render Thread
=====================
*/

enum class RENDER_THREAD_EVENT_TYPE
{
	RENDER_THREAD_PROCESS,
	RENDER_THREAD_EXIT,
	RENDER_THREAD_TYPE_COUNT,
};

struct RENDER_THREAD_DESC
{
	HANDLE threadEvent[static_cast<uint32>(RENDER_THREAD_EVENT_TYPE::RENDER_THREAD_TYPE_COUNT)] = {};
	HANDLE threadHandle = nullptr;
	void* renderBody = nullptr;
	uint32 threadIdx = 0;
};

class RenderThread
{
public:
	static uint32 ProcessByRenderThread(void* param);
};
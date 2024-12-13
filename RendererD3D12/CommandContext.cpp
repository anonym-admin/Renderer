#include "pch.h"
#include "CommandContext.h"

/*
====================
CommandContext
====================
*/

CommandContext::CommandContext()
{
}

CommandContext::~CommandContext()
{
	CleanUp();
}

bool CommandContext::Initialize(ID3D12Device5* device, uint32 maxNumCmdList)
{
	if (maxNumCmdList < 2)
	{
		__debugbreak();
	}

	m_device = device;
	m_maxNumCmdList = maxNumCmdList;

	return true;
}

COMMAND_CONTEXT_HANDLE* CommandContext::AllocCmdCtx()
{
	COMMAND_CONTEXT_HANDLE* availCmdCtx = reinterpret_cast<COMMAND_CONTEXT_HANDLE*>(m_availCmdCtxHead);

	if (!availCmdCtx)
	{
		availCmdCtx = AddCmdCtx();
	}
	else
	{
		DL_LIST* link = reinterpret_cast<DL_LIST*>(availCmdCtx);
		DL_Delete(&m_availCmdCtxHead, &m_availCmdCtxTail, link);
	}

	DL_LIST* link = reinterpret_cast<DL_LIST*>(availCmdCtx);
	DL_InsertBack(&m_usedCmdCtxHead, &m_usedCmdCtxTail, link);

	m_allocNum++;

	return availCmdCtx;
}

void CommandContext::Free()
{
	m_allocNum = 0;

	DL_LIST* cur = m_usedCmdCtxHead;
	while (cur != nullptr)
	{
		DL_LIST* next = cur->next;
		COMMAND_CONTEXT_HANDLE* cmdCtxHandle = reinterpret_cast<COMMAND_CONTEXT_HANDLE*>(cur);
		DL_Delete(&m_usedCmdCtxHead, &m_usedCmdCtxTail, cur);
		DL_InsertBack(&m_availCmdCtxHead, &m_availCmdCtxTail, cur);

		ThrowIfFailed(cmdCtxHandle->cmdAllocator->Reset());
		ThrowIfFailed(cmdCtxHandle->cmdList->Reset(cmdCtxHandle->cmdAllocator, nullptr));

		cur = next;
	}
}

void CommandContext::Close()
{
	if (!m_curCmdCtxHandle)
	{
		__debugbreak();
	}
	if (!m_curCmdCtxHandle->cmdList)
	{
		__debugbreak();
	}

	ID3D12GraphicsCommandList* cmdList = m_curCmdCtxHandle->cmdList;

	ThrowIfFailed(cmdList->Close());

	m_curCmdCtxHandle = nullptr;
}

void CommandContext::CloseAndExcute(ID3D12CommandQueue* cmdQueue)
{
	ID3D12GraphicsCommandList* cmdList = m_curCmdCtxHandle->cmdList;

	if (!cmdList)
	{
		__debugbreak();
	}
	
	ThrowIfFailed(cmdList->Close());
	ID3D12CommandList* cmdLists[] = { cmdList };
	cmdQueue->ExecuteCommandLists(1, cmdLists);

	m_curCmdCtxHandle = nullptr;
}

ID3D12GraphicsCommandList* CommandContext::GetCurrentCommandList()
{
	COMMAND_CONTEXT_HANDLE* cmdCtxHandle = nullptr;

	if (!m_curCmdCtxHandle)
	{
		cmdCtxHandle = AllocCmdCtx();
		if (!cmdCtxHandle)
		{
			__debugbreak();
		}

		m_curCmdCtxHandle = cmdCtxHandle;
	}

	return m_curCmdCtxHandle->cmdList;
}

void CommandContext::CleanUp()
{
	DL_LIST* cur = m_usedCmdCtxHead;
	while (cur != nullptr)
	{
		DL_LIST* next = cur->next;
		COMMAND_CONTEXT_HANDLE* cmdCtxHandle = reinterpret_cast<COMMAND_CONTEXT_HANDLE*>(cur);
		DL_Delete(&m_usedCmdCtxHead, &m_usedCmdCtxTail, cur);

		DeleteCmdCtx(cmdCtxHandle);

		cur = next;
	}

	cur = m_availCmdCtxHead;
	while (cur != nullptr)
	{
		DL_LIST* next = cur->next;
		COMMAND_CONTEXT_HANDLE* cmdCtxHandle = reinterpret_cast<COMMAND_CONTEXT_HANDLE*>(cur);
		DL_Delete(&m_availCmdCtxHead, &m_availCmdCtxTail, cur);

		DeleteCmdCtx(cmdCtxHandle);

		cur = next;
	}
}

COMMAND_CONTEXT_HANDLE* CommandContext::AddCmdCtx()
{
	if (m_allocNum >= m_maxNumCmdList)
	{
		__debugbreak();
	}

	ID3D12GraphicsCommandList* cmdList = nullptr;
	ID3D12CommandAllocator* cmdAllocator = nullptr;
	COMMAND_CONTEXT_HANDLE* cmdCtxHandle = new COMMAND_CONTEXT_HANDLE;

	// Create the command allocator.
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList)));

	cmdCtxHandle->cmdAllocator = cmdAllocator;
	cmdCtxHandle->cmdList = cmdList;

	m_cmdListNum++;

	return cmdCtxHandle;
}

void CommandContext::DeleteCmdCtx(COMMAND_CONTEXT_HANDLE* cmdCtxHandle)
{
	if (cmdCtxHandle)
	{
		if (cmdCtxHandle->cmdList)
		{
			cmdCtxHandle->cmdList->Release();
			cmdCtxHandle->cmdList = nullptr;
		}
		if (cmdCtxHandle->cmdAllocator)
		{
			cmdCtxHandle->cmdAllocator->Release();
			cmdCtxHandle->cmdAllocator = nullptr;
		}

		delete cmdCtxHandle;
		cmdCtxHandle = nullptr;
	}
}

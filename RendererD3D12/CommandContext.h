#pragma once

/*
====================
CommandContext
====================
*/

class CommandContext
{
public:
	CommandContext();
	~CommandContext();

	bool Initialize(ID3D12Device5* device, uint32 maxNumCmdList);
	COMMAND_CONTEXT_HANDLE* AllocCmdCtx();
	void Free();
	void Close();
	void CloseAndExcute(ID3D12CommandQueue* cmdQueue);
	ID3D12GraphicsCommandList* GetCurrentCommandList();
	inline uint32 GetCmdListCount() { return m_cmdListNum; }

private:
	void CleanUp();
	COMMAND_CONTEXT_HANDLE* AddCmdCtx();
	void DeleteCmdCtx(COMMAND_CONTEXT_HANDLE* cmdCtxHandle);

private:
	ID3D12Device* m_device = nullptr;
	DL_LIST* m_availCmdCtxHead = nullptr;
	DL_LIST* m_availCmdCtxTail = nullptr;
	DL_LIST* m_usedCmdCtxHead = nullptr;
	DL_LIST* m_usedCmdCtxTail = nullptr;
	COMMAND_CONTEXT_HANDLE* m_curCmdCtxHandle = nullptr;
	uint32 m_maxNumCmdList = 0;
	uint32 m_allocNum = 0;
	uint32 m_cmdListNum = 0;
};


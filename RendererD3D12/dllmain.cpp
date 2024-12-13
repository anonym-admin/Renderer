#include "pch.h"
#include "Renderer.h"

// directx lib
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
// my lib
#pragma comment(lib, "CommonLib.lib")

/*
===========
Main Entry
===========
*/

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
#ifdef _DEBUG
        int32 flags = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(flags);
#endif
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    {
#ifdef _DEBUG
        _ASSERT(_CrtCheckMemory());
#endif
    }
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/*
====================
Create Renderer Dll
====================
*/

void Dll_CreateInstance(void** inst)
{
    IT_Renderer** itRenderer = (IT_Renderer**)inst;
    if (*itRenderer)
    {
        __debugbreak();
    }
    *itRenderer = new Renderer;
}
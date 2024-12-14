#pragma once

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
	#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// run time lib
#include <stdio.h>
#include <windows.h>
#include <process.h>
// directx
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_3.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
using namespace DirectX;
// my
#include "../../Common/Type.h"
#include "../../CommonLib/CommonLib/LinkedList.h"
#include "../../CommonLib/CommonLib/HashTable.h"
#include "../../GenericUtils.h"
#include "D3DUtils.h"
#include "RendererType.h"
#include "RenderThread.h"

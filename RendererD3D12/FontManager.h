#pragma once

/*
===============
FontManager
===============
*/

class Renderer;

class FontManager
{
public:
	FontManager();
	~FontManager();

	bool Initialize(Renderer* renderer, ID3D12CommandQueue* cmdQueue, uint32 width, uint32 height, bool enableDebugLayer);

private:
	void CleanUp();
	bool CreateD2D(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, bool enableDebugLayer);
	bool CreateDWrite(ID3D12Device* device, uint32 width, uint32 height, float dpi);

private:
	ID2D1Device2* m_d2dDevice = nullptr;
	ID2D1DeviceContext2* m_d2dDeviceContext = nullptr;
};


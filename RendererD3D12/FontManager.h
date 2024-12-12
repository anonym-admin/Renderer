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
	FONT_HANDLE* CreateFontObject(const wchar_t* fontName, float fontSize);
	void CreateBitmapFromText(int32* texWidth, int32* texHeight, IDWriteTextFormat* textFormat, const wchar_t* contentsString, uint32 strLen);
	void WriteTextToBitmap(uint8* destImage, uint32 destWidth, uint32 destHeight, int32* texWidth, int32* texHeight, FONT_HANDLE* fontHandle, const wchar_t* contentsString, uint32 strLen);


private:
	void CleanUp();
	bool CreateD2D(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, bool enableDebugLayer);
	bool CreateDWrite(ID3D12Device* device, uint32 width, uint32 height, float dpi);

private:
	ID2D1Device2* m_d2dDevice = nullptr;
	ID2D1DeviceContext2* m_d2dDeviceContext = nullptr;
	ID2D1Bitmap1* m_d2dTargetBitmap = nullptr;
	ID2D1Bitmap1* m_d2dTargetBitmapRead = nullptr;
	ID2D1SolidColorBrush* m_whiteBrush = nullptr;
	IDWriteFactory5* m_dwFactory = nullptr;
	uint32 m_bitmapWidth = 0;
	uint32 m_bitmapHeight = 0;
};


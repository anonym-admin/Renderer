#include "pch.h"
#include "FontManager.h"
#include "Renderer.h"

using namespace D2D1;

/*
===============
FontManager
===============
*/

FontManager::FontManager()
{
}

FontManager::~FontManager()
{
	CleanUp();
}

bool FontManager::Initialize(Renderer* renderer, ID3D12CommandQueue* cmdQueue, uint32 width, uint32 height, bool enableDebugLayer)
{
	ID3D12Device* device = renderer->GetDevice();

	if (!CreateD2D(device, cmdQueue, enableDebugLayer))
	{
		return false;
	}

	float dpi = renderer->GetDpi();

	if (!CreateDWrite(device, width, height, dpi))
	{
		return false;
	}

	return true;
}

FONT_HANDLE* FontManager::CreateFontObject(const wchar_t* fontName, float fontSize)
{
	IDWriteTextFormat* textFormat = nullptr;
	IDWriteFactory5* dwFactory = m_dwFactory;
	IDWriteFontCollection1* fontCollection = nullptr;

	if (dwFactory)
	{
		ThrowIfFailed(dwFactory->CreateTextFormat(fontName, fontCollection, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, DEFULAT_LOCALE_NAME, &textFormat));
	}

	FONT_HANDLE* fontHandle = new FONT_HANDLE;
	memset(fontHandle, 0, sizeof(FONT_HANDLE));
	wcscpy_s(fontHandle->fontName, fontName);
	fontHandle->fontSize = fontSize;

	if (textFormat)
	{
		ThrowIfFailed(textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));

		ThrowIfFailed(textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));
	}

	fontHandle->textFormat = textFormat;

	return fontHandle;
}

void FontManager::CreateBitmapFromText(int32* texWidth, int32* texHeight, IDWriteTextFormat* textFormat, const wchar_t* contentsString, uint32 strLen)
{
	ID2D1DeviceContext* d2dDeviceContext = m_d2dDeviceContext;
	IDWriteFactory5* dwFactory = m_dwFactory;
	D2D1_SIZE_F maxSize = d2dDeviceContext->GetSize();
	maxSize.width = static_cast<float>(m_bitmapWidth);
	maxSize.height = static_cast<float>(m_bitmapHeight);

	IDWriteTextLayout* textLayout = nullptr;
	if (dwFactory && textFormat)
	{
		ThrowIfFailed(dwFactory->CreateTextLayout(contentsString, strLen, textFormat, maxSize.width, maxSize.height, &textLayout));
	}

	DWRITE_TEXT_METRICS metrics = {};
	if (textLayout)
	{
		textLayout->GetMetrics(&metrics);

		d2dDeviceContext->SetTarget(m_d2dTargetBitmap);

		d2dDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

		d2dDeviceContext->BeginDraw();

		d2dDeviceContext->Clear(ColorF(ColorF::Black));
		d2dDeviceContext->SetTransform(Matrix3x2F::Identity());

		d2dDeviceContext->DrawTextLayout(Point2F(0.0f, 0.0f), textLayout, m_whiteBrush);

		// We ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
		// is lost. It will be handled during the next call to Present.
		d2dDeviceContext->EndDraw();

		d2dDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
		d2dDeviceContext->SetTarget(nullptr);

		textLayout->Release();
		textLayout = nullptr;
	}

	int32 width = static_cast<int32>(ceil(metrics.width));
	int32 height = static_cast<int32>(ceil(metrics.height));

	D2D1_POINT_2U destPos = {};
	D2D1_RECT_U srcRect = { 0, 0, static_cast<uint32>(width), static_cast<uint32>(height) };
	ThrowIfFailed(m_d2dTargetBitmapRead->CopyFromBitmap(&destPos, m_d2dTargetBitmap, &srcRect));

	*texWidth = width;
	*texHeight = height;
}

void FontManager::WriteTextToBitmap(uint8* destImage, uint32 destWidth, uint32 destHeight, uint32 destPitch, int32* texWidth, int32* texHeight, FONT_HANDLE* fontHandle, const wchar_t* contentsString, uint32 strLen)
{
	int32 textureWidth = 0;
	int32 textureHeight = 0;

	CreateBitmapFromText(&textureWidth, &textureHeight, fontHandle->textFormat, contentsString, strLen);

	if (textureWidth > static_cast<int32>(destWidth))
	{
		textureWidth = static_cast<int32>(destWidth);
	}
	if (textureHeight > static_cast<int32>(destHeight))
	{
		textureHeight = static_cast<int32>(destHeight);
	}

	D2D1_MAPPED_RECT  mappedRect;
	ThrowIfFailed(m_d2dTargetBitmapRead->Map(D2D1_MAP_OPTIONS_READ, &mappedRect));

	uint8* dest = destImage;
	uint8* src = reinterpret_cast<uint8*>(mappedRect.bits);

	for (uint32 h = 0; h < static_cast<uint32>(textureHeight); h++)
	{
		memcpy(dest, src, textureWidth * 4);
		dest += destPitch;
		src += mappedRect.pitch;
	}

	m_d2dTargetBitmapRead->Unmap();

	*texWidth = textureWidth;
	*texHeight = textureHeight;
}

void FontManager::DestroyFontObject(FONT_HANDLE* fontHandle)
{
	if (fontHandle)
	{
		if (fontHandle->textFormat)
		{
			fontHandle->textFormat->Release();
			fontHandle->textFormat = nullptr;
		}

		delete fontHandle;
		fontHandle = nullptr;
	}
}

void FontManager::CleanUp()
{
	if (m_dwFactory)
	{
		m_dwFactory->Release();
		m_dwFactory = nullptr;
	}
	if (m_whiteBrush)
	{
		m_whiteBrush->Release();
		m_whiteBrush = nullptr;
	}
	if (m_d2dTargetBitmapRead)
	{
		m_d2dTargetBitmapRead->Release();
		m_d2dTargetBitmapRead = nullptr;
	}
	if (m_d2dTargetBitmap)
	{
		m_d2dTargetBitmap->Release();
		m_d2dTargetBitmap = nullptr;
	}
	if (m_d2dDeviceContext)
	{
		m_d2dDeviceContext->Release();
		m_d2dDeviceContext = nullptr;
	}
	if (m_d2dDevice)
	{
		m_d2dDevice->Release();
		m_d2dDevice = nullptr;
	}
}

bool FontManager::CreateD2D(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, bool enableDebugLayer)
{
	uint32 d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};

	ID2D1Factory3* d2dFactory = nullptr;
	ID3D11Device* d3d11Device = nullptr;
	ID3D11DeviceContext* d3d11DeviceContext = nullptr;
	ID3D11On12Device* d3d11On12Device = nullptr;
	IDXGIDevice* dxgiDevice = nullptr;

	if (enableDebugLayer)
	{
		d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
	d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

	ThrowIfFailed(D3D11On12CreateDevice(device, d3d11DeviceFlags, nullptr, 0, (IUnknown**)&cmdQueue, 1, 0, &d3d11Device, &d3d11DeviceContext, nullptr));
	ThrowIfFailed(d3d11Device->QueryInterface(IID_PPV_ARGS(&d3d11On12Device)));

	// Create D2D/DWrite components.
	D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, (void**)&d2dFactory));
	ThrowIfFailed(d3d11On12Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
	ThrowIfFailed(d2dFactory->CreateDevice(dxgiDevice, &m_d2dDevice));
	ThrowIfFailed(m_d2dDevice->CreateDeviceContext(deviceOptions, &m_d2dDeviceContext));

	if (dxgiDevice)
	{
		dxgiDevice->Release();
		dxgiDevice = nullptr;
	}
	if (d3d11On12Device)
	{
		d3d11On12Device->Release();
		d3d11On12Device = nullptr;
	}
	if (d3d11DeviceContext)
	{
		d3d11DeviceContext->Release();
		d3d11DeviceContext = nullptr;
	}
	if (d3d11Device)
	{
		d3d11Device->Release();
		d3d11Device = nullptr;
	}
	if (d2dFactory)
	{
		d2dFactory->Release();
		d2dFactory = nullptr;
	}

	return true;
}

bool FontManager::CreateDWrite(ID3D12Device* device, uint32 width, uint32 height, float dpi)
{
	m_bitmapWidth = width;
	m_bitmapHeight = height;

	D2D1_SIZE_U	size;
	size.width = width;
	size.height = height;

	D2D1_BITMAP_PROPERTIES1 bitmapProperties = BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		dpi,
		dpi);

	ThrowIfFailed(m_d2dDeviceContext->CreateBitmap(size, nullptr, 0, &bitmapProperties, &m_d2dTargetBitmap));

	bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ;
	ThrowIfFailed(m_d2dDeviceContext->CreateBitmap(size, nullptr, 0, &bitmapProperties, &m_d2dTargetBitmapRead));

	ThrowIfFailed(m_d2dDeviceContext->CreateSolidColorBrush(ColorF(ColorF::White), &m_whiteBrush));

	ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5), (IUnknown**)&m_dwFactory));

	return true;
}

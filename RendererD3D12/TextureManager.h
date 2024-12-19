#pragma once

/*
=================
TextureManager
=================
*/

class Renderer;
struct HashTable;

class TextureManager
{
public:
	TextureManager();
	~TextureManager();

	bool Initialize(Renderer* renderer);
	TEXTURE_HANDLE* CreateTiledTexture(uint32 texWidth, uint32 texHeight, uint32 cellWidth, uint32 cellHeight);
	TEXTURE_HANDLE* CreateTextureFromFile(const wchar_t* filename);
	TEXTURE_HANDLE* CreateDynamicTexture(uint32 texWidth, uint32 texHeight, const char* name = nullptr);
	TEXTURE_HANDLE* CreateDummyTexture(uint32 texWidth = 1, uint32 texHeight = 1);
	void DestroyTexture(TEXTURE_HANDLE* textureHandle);
	
private:
	void CleanUp();

private:
	Renderer* m_renderer = nullptr;
	HashTable* m_hashTable = nullptr;
};


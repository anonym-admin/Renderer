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
	TEXTURE_HANDLE* CreateTextureFromFile(const wchar_t* filename);
	TEXTURE_HANDLE* CreateDynamicTexture(uint32 texWidth, uint32 texHeight);
	void DestroyTexture(TEXTURE_HANDLE* textureHandle);
	
private:
	void CleanUp();

private:
	Renderer* m_renderer = nullptr;
	HashTable* m_hashTable = nullptr;
};


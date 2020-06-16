#pragma once
#include "DescriptorHeap.h"
#include "d3dUtil.h"

class TextureMap
{
public:
	void Add(ID3D12Device* d3dDevice, ID3D12GraphicsCommandList* cmdList, const DescriptorHeap& heap, const std::string& name, const std::wstring& filename);

private:
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

};


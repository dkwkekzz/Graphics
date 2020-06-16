#pragma once
#include "CD3D12.h"
#include "Camera.h"
#include "DescriptorHeap.h"
#include "TextureMap.h"

class World
{
public:
	inline CD3D12& GetD3D() { return m_d3d; }
	inline Camera& GetCamera() { return m_cam; }
	inline DescriptorHeap& GetDescriptorHeap() { return m_desc; }
	inline TextureMap& GetTextureMap() { return m_texMap; }

private:
	CD3D12 m_d3d;
	Camera m_cam;
	DescriptorHeap m_desc;
	TextureMap m_texMap;

};
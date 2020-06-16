#pragma once
#include "d3dUtil.h"

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexC;
};

class GeometryMap
{
public:
	void Init(ID3D12Device* d3dDevice, ID3D12GraphicsCommandList* cmdList);

private:
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;

};


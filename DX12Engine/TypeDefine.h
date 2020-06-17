#pragma once
#include "pch.h"

using TPSO = ID3D12PipelineState;
using TShader = ID3DBlob;
using TResource = ID3D12Resource;
using TSignature = ID3D12RootSignature;
using TDevice = ID3D12Device;
using Matrix = DirectX::XMFLOAT4X4;

enum class RenderLayer : int
{
	Opaque = 0,
	Mirrors,
	Reflected,
	Transparent,
	Shadow,
	Count
};

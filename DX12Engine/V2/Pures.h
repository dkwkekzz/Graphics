#pragma once
#include "pch.h"
#include "SLO.h"


inline static ID3D12Resource* CurrentBackBuffer(const SLO::GL* pGL)
{
	return pGL->swapChainBuffer[pGL->currBackBuffer].Get();
}

inline static bool ValidDevice(const SLO::GL* pGL)
{
	return pGL->d3dDevice != nullptr;
}

inline static D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView(const SLO::DescriptorHeap* pDescriptorHeap, const SLO::GL* pGL)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		pDescriptorHeap->rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		pGL->currBackBuffer,
		pDescriptorHeap->rtvDescriptorSize);
}

inline static D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView(const SLO::DescriptorHeap* pDescriptorHeap)
{
	return pDescriptorHeap->dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

inline static CD3DX12_CPU_DESCRIPTOR_HANDLE SrvHeapHandle(const SLO::DescriptorHeap* pDescriptorHeap)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(pDescriptorHeap->srvHeap->GetCPUDescriptorHandleForHeapStart());
}

inline static CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHeapHandle(const SLO::DescriptorHeap* pDescriptorHeap)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(pDescriptorHeap->rtvHeap->GetCPUDescriptorHandleForHeapStart());
}

inline static CD3DX12_CPU_DESCRIPTOR_HANDLE DsvHeapHandle(const SLO::DescriptorHeap* pDescriptorHeap)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(pDescriptorHeap->dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

inline static float GameTotalTime(const GameTimer* pGameTimer)
{
	return pGameTimer->TotalTime();
}

static XMMATRIX CalcMatrix(float t[3], float r[3], float s[3])
{
	XMMATRIX ret = XMLoadFloat4x4(&MathHelper::Identity4x4());

	if (nullptr != r)
	{
		if (r[0] != 0.0f)
			ret *= XMMatrixRotationX(r[0]);
		if (r[1] != 0.0f)
			ret *= XMMatrixRotationY(r[1]);
		if (r[2] != 0.0f)
			ret *= XMMatrixRotationZ(r[2]);
	}

	if (nullptr != s)
	{
		ret *= XMMatrixScaling(s[0], s[1], s[2]);
	}

	if (nullptr != t)
	{
		ret *= XMMatrixTranslation(t[0], t[1], t[2]);
	}

	return ret;
}

static float GetHillsHeight(float x, float z)
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

static XMFLOAT3 GetHillsNormal(float x, float z)
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}
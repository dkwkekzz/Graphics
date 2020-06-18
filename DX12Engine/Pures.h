#pragma once
#include "pch.h"
#include "SLO.h"

namespace SLP
{
	inline static ID3D12Resource* CurrentBackBuffer(const SLO::GL* pGL)
	{
		return pGL->swapChainBuffer[pGL->currBackBuffer].Get();
	}

	inline static bool ValidDevice(const SLO::GL* pGL)
	{
		return pGL->d3dDevice != nullptr;
	}

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE BackBufferView(const SLO::DescriptorHeap* pDescriptorHeap, int mCurrBackBuffer)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(pDescriptorHeap->rtvHeap->GetCPUDescriptorHandleForHeapStart(), 
			mCurrBackBuffer, pDescriptorHeap->rtvDescriptorSize);
	}

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE DepthStencilView(const SLO::DescriptorHeap* pDescriptorHeap)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(pDescriptorHeap->dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE SrvHeapHandle(const SLO::DescriptorHeap* pDescriptorHeap)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(pDescriptorHeap->srvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHeapHandle(const SLO::DescriptorHeap* pDescriptorHeap)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(pDescriptorHeap->rtvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE DsvHeapHandle(const SLO::DescriptorHeap* pDescriptorHeap)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(pDescriptorHeap->dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}
}
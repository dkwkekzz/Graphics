#pragma once
#include "pch.h"
#include "GlobalVar.h"
#include "d3dx12.h"
#include "d3dUtil.h"

namespace Properties
{
	struct GL
	{
		Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
		Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
		D3D_DRIVER_TYPE d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;

		//Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
		//Microsoft::WRL::ComPtr<ID3D12CommandAllocator> directCmdListAlloc;
		//Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
		//
		//Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		//UINT64 currentFence = 0;

		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
		Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffer[Global::SWAP_CHAIN_BUFFER_COUNT];
		int currBackBuffer = 0;
		DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;
		DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// Set true to use 4X MSAA (?.1.8).  The default is false.
		bool Msaa4xState = false;    // 4X MSAA enabled
		UINT Msaa4xQuality = 0;      // quality level of 4X MSAA

		D3D12_VIEWPORT screenViewport;
		D3D12_RECT scissorRect;
		int clientWidth = Global::SCREEN_WIDTH;
		int clientHeight = Global::SCREEN_HEIGHT;

		UINT rtvDescriptorSize = 0;
		UINT dsvDescriptorSize = 0;
		UINT cbvSrvUavDescriptorSize = 0;

		HWND mainWnd;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;

		inline ID3D12Resource* CurrentBackBuffer() const 
		{
			return swapChainBuffer[currBackBuffer].Get(); 
		}

		inline D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currBackBuffer, rtvDescriptorSize);
		}

		inline D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const
		{
			return dsvHeap->GetCPUDescriptorHandleForHeapStart();
		}
	};

	struct CommandObject
	{
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> directCmdListAlloc;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		UINT64 currentFence = 0;

		inline void Reset(ID3D12PipelineState* initialState)
		{
			// Reset the command list to prep for initialization commands.
			ThrowIfFailed(commandList->Reset(directCmdListAlloc.Get(), initialState));
		}

		void Signal()
		{
			// Advance the fence value to mark commands up to this fence point.
			currentFence++;

			// Notify the fence when the GPU completes commands up to this fence point.
			commandQueue->Signal(fence.Get(), currentFence);
		}

		void Wait()
		{
			// Wait until the GPU has completed commands up to this fence point.
			if (fence->GetCompletedValue() < currentFence)
			{
				HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

				// Fire event when GPU hits current fence.  
				ThrowIfFailed(fence->SetEventOnCompletion(currentFence, eventHandle));

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}
		}

		// Execute the resize commands.
		void Execute()
		{
			ThrowIfFailed(commandList->Close());
			ID3D12CommandList* cmdsLists[] = { commandList.Get() };
			commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
		}

		// Wait until complete.
		void Flush()
		{
			Signal();
			Wait();
		}
	};

	struct RootSignature
	{
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

		void Build()
		{

		}
	};
}

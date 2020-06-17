#pragma once
#include "pch.h"
#include "Properties.h"
#include "GlobalVar.h"
#include "d3dx12.h"
#include "d3dUtil.h"

namespace Processes
{
	using namespace Microsoft::WRL;

	// construct

	static void InitializeDevice(Properties::GL* pGL, Properties::CommandObject* pCommandObject)
	{
#if defined(DEBUG) || defined(_DEBUG) 
		// Enable the D3D12 debug layer.
		{
			ComPtr<ID3D12Debug> debugController;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
		}
#endif

		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&pGL->dxgiFactory)));

		// Try to create hardware device.
		HRESULT hardwareResult = D3D12CreateDevice(
			nullptr,             // default adapter
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&pGL->d3dDevice));

		// Fallback to WARP device.
		if (FAILED(hardwareResult))
		{
			ComPtr<IDXGIAdapter> pWarpAdapter;
			ThrowIfFailed(pGL->dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

			ThrowIfFailed(D3D12CreateDevice(
				pWarpAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&pGL->d3dDevice)));
		}

		ThrowIfFailed(pGL->d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&pCommandObject->fence)));

		pGL->rtvDescriptorSize = pGL->d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		pGL->dsvDescriptorSize = pGL->d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		pGL->cbvSrvUavDescriptorSize = pGL->d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Check 4X MSAA quality support for our back buffer format.
		// All Direct3D 11 capable devices support 4X MSAA for all render 
		// target formats, so we only need to check quality support.

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
		msQualityLevels.Format = pGL->backBufferFormat;
		msQualityLevels.SampleCount = 4;
		msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		msQualityLevels.NumQualityLevels = 0;
		ThrowIfFailed(pGL->d3dDevice->CheckFeatureSupport(
			D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
			&msQualityLevels,
			sizeof(msQualityLevels)));

		pGL->Msaa4xQuality = msQualityLevels.NumQualityLevels;
		assert(pGL->Msaa4xQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
		//LogAdapters();
#endif

		//CreateCommandObjects();
		//CreateSwapChain();
		//CreateRtvAndDsvDescriptorHeaps();

	}

	static void CreateCommandObjects(Properties::GL* pGL, Properties::CommandObject* pCommandObject)
	{
		auto* d3dDevice = pGL->d3dDevice.Get();

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandObject->commandQueue)));

		ThrowIfFailed(d3dDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(pCommandObject->directCmdListAlloc.GetAddressOf())));

		ThrowIfFailed(d3dDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			pCommandObject->directCmdListAlloc.Get(), // Associated command allocator
			nullptr,                   // Initial PipelineStateObject
			IID_PPV_ARGS(pCommandObject->commandList.GetAddressOf())));

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		pCommandObject->commandList->Close();
	}

	static void CreateSwapChain(Properties::GL* pGL, Properties::CommandObject* pCommandObject)
	{
		// Release the previous swapchain we will be recreating.
		pGL->swapChain.Reset();

		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = pGL->clientWidth;
		sd.BufferDesc.Height = pGL->clientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = pGL->backBufferFormat;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = pGL->Msaa4xState ? 4 : 1;
		sd.SampleDesc.Quality = pGL->Msaa4xState ? (pGL->Msaa4xQuality - 1) : 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = Global::SWAP_CHAIN_BUFFER_COUNT;
		sd.OutputWindow = pGL->mainWnd;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// Note: Swap chain uses queue to perform flush.
		ThrowIfFailed(pGL->dxgiFactory->CreateSwapChain(
			pCommandObject->commandQueue.Get(),
			&sd,
			pGL->swapChain.GetAddressOf()));
	}

	static void CreateRtvAndDsvDescriptorHeaps(Properties::GL* pGL)
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = Global::SWAP_CHAIN_BUFFER_COUNT;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		ThrowIfFailed(pGL->d3dDevice->CreateDescriptorHeap(
			&rtvHeapDesc, IID_PPV_ARGS(pGL->rtvHeap.GetAddressOf())));


		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		ThrowIfFailed(pGL->d3dDevice->CreateDescriptorHeap(
			&dsvHeapDesc, IID_PPV_ARGS(pGL->dsvHeap.GetAddressOf())));
	}

	static void CreateSrvDescriptorHeap(Properties::GL* pGL)
	{
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = Global::MAX_TEXTURE_COUNT;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(pGL->d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&pGL->srvHeap)));
	}

	static void Resize(Properties::GL* pGL, Properties::CommandObject* pCommandObject)
	{
		assert(pGL->d3dDevice);
		assert(pGL->swapChain);
		assert(pCommandObject->directCmdListAlloc);

		// Flush before changing any resources.
		pCommandObject->Flush();

		pCommandObject->Reset(nullptr);

		// Release the previous resources we will be recreating.
		for (int i = 0; i < Global::SWAP_CHAIN_BUFFER_COUNT; ++i)
			pGL->swapChainBuffer[i].Reset();
		pGL->depthStencilBuffer.Reset();

		// Resize the swap chain.
		ThrowIfFailed(pGL->swapChain->ResizeBuffers(
			Global::SWAP_CHAIN_BUFFER_COUNT,
			pGL->clientWidth, pGL->clientHeight,
			pGL->backBufferFormat,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		pGL->currBackBuffer = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(pGL->rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < Global::SWAP_CHAIN_BUFFER_COUNT; i++)
		{
			ThrowIfFailed(pGL->swapChain->GetBuffer(i, IID_PPV_ARGS(&pGL->swapChainBuffer[i])));
			pGL->d3dDevice->CreateRenderTargetView(pGL->swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
			rtvHeapHandle.Offset(1, pGL->rtvDescriptorSize);
		}

		// Create the depth/stencil buffer and view.
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = pGL->clientWidth;
		depthStencilDesc.Height = pGL->clientHeight;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;

		// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
		// the depth buffer.  Therefore, because we need to create two views to the same resource:
		//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
		//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
		// we need to create the depth buffer resource with a typeless format.  
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

		depthStencilDesc.SampleDesc.Count = pGL->Msaa4xState ? 4 : 1;
		depthStencilDesc.SampleDesc.Quality = pGL->Msaa4xState ? (pGL->Msaa4xQuality - 1) : 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = pGL->depthStencilFormat;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;
		ThrowIfFailed(pGL->d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(pGL->depthStencilBuffer.GetAddressOf())));

		// Create descriptor to mip level 0 of entire resource using the format of the resource.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = pGL->depthStencilFormat;
		dsvDesc.Texture2D.MipSlice = 0;
		pGL->d3dDevice->CreateDepthStencilView(pGL->depthStencilBuffer.Get(), &dsvDesc, pGL->DepthStencilView());

		// Transition the resource from its initial state to be used as a depth buffer.
		pCommandObject->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pGL->depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		// Execute the resize commands.
		pCommandObject->Execute();

		// Wait until resize is complete.
		pCommandObject->Flush();

		// Update the viewport transform to cover the client area.
		pGL->screenViewport.TopLeftX = 0;
		pGL->screenViewport.TopLeftY = 0;
		pGL->screenViewport.Width = static_cast<float>(pGL->clientWidth);
		pGL->screenViewport.Height = static_cast<float>(pGL->clientHeight);
		pGL->screenViewport.MinDepth = 0.0f;
		pGL->screenViewport.MaxDepth = 1.0f;

		pGL->scissorRect = { 0, 0, pGL->clientWidth, pGL->clientHeight };
	}

	static void InitializeAdvanced(Properties::GL* pGL, Properties::CommandObject* pCommandObject)
	{
		pCommandObject->Reset(nullptr);

		LoadTextures();
		BuildRootSignature();
		BuildDescriptorHeaps();
		BuildShadersAndInputLayout();
		BuildRoomGeometry();
		BuildSkullGeometry();
		BuildMaterials();
		BuildRenderItems();
		BuildFrameResources();
		BuildPSOs();

		// Execute the resize commands.
		pCommandObject->Execute();

		// Wait until resize is complete.
		pCommandObject->Flush();
	}

}
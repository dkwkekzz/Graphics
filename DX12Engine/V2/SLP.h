#pragma once
#include "pch.h"
#include "SLO.h"
#include "GlobalVar.h"
#include "d3dx12.h"
#include "d3dUtil.h"
#include "SLP2G.h"
#include "Pures.h"
#include "string_utils.h"

namespace SLP
{
	static const UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(SLO::ObjectConstants));
	static const UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(SLO::PassConstants));
	static const UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(SLO::MaterialConstants));

	struct GCommander
	{
		ROOTCALL ResetDirectly(SLO::CommandObject* pCommandObject)
		{
			// Reset the command list to prep for initialization commands.
			ThrowIfFailed(pCommandObject->commandList->Reset(pCommandObject->directCmdListAlloc.Get(), nullptr));
		}

		ROOTCALL ResetByCurrFrame(SLO::CommandObject* pCommandObject, SLO::ResourceManager* pResourceManager)
		{
			auto* currFrame = pResourceManager->currFrameResource;

			// Reuse the memory associated with command recording.
			// We can only reset when the associated command lists have finished execution on the GPU.
			ThrowIfFailed(currFrame->CmdListAlloc->Reset());

			// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
			// Reusing the command list reuses memory.
			ThrowIfFailed(pCommandObject->commandList->Reset(currFrame->CmdListAlloc.Get(), nullptr));
		}

		ROOTCALL Signal(SLO::CommandObject* pCommandObject)
		{
			// Advance the fence value to mark commands up to this fence point.
			pCommandObject->currentFence++;

			// Notify the fence when the GPU completes commands up to this fence point.
			pCommandObject->commandQueue->Signal(pCommandObject->fence.Get(), pCommandObject->currentFence);
		}

		ROOTCALL SignalByCurrFrame(SLO::CommandObject* pCommandObject, SLO::ResourceManager* pResourceManager)
		{
			// Advance the fence value to mark commands up to this fence point.
			pResourceManager->currFrameResource->Fence = ++pCommandObject->currentFence;

			// Notify the fence when the GPU completes commands up to this fence point.
			pCommandObject->commandQueue->Signal(pCommandObject->fence.Get(), pCommandObject->currentFence);
		}

		ROOTCALL Wait(SLO::CommandObject* pCommandObject)
		{
			// Wait until the GPU has completed commands up to this fence point.
			if (pCommandObject->fence->GetCompletedValue() < pCommandObject->currentFence)
			{
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, FALSE, EVENT_ALL_ACCESS);

				// Fire event when GPU hits current fence.  
				ThrowIfFailed(pCommandObject->fence->SetEventOnCompletion(pCommandObject->currentFence, eventHandle));

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}
		}

		// Execute the resize commands.
		ROOTCALL Execute(SLO::CommandObject* pCommandObject)
		{
			ThrowIfFailed(pCommandObject->commandList->Close());
			ID3D12CommandList* cmdsLists[] = { pCommandObject->commandList.Get() };
			pCommandObject->commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
		}

		// Wait until complete.
		ROOTCALL Flush(SLO::CommandObject* pCommandObject)
		{
			Signal(pCommandObject);
			Wait(pCommandObject);
		}
	};

	struct GConstruct
	{
		ROOTCALL CreateHandle(SLO::GL* pGL, HWND mainWnd, int width, int height)
		{
			pGL->mainWnd = mainWnd;
			pGL->clientWidth = width;
			pGL->clientHeight = height;
		}

		ROOTCALL CreateDevice(SLO::GL* pGL, SLO::CommandObject* pCommandObject)
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

		ROOTCALL CreateCommandObjects(SLO::GL* pGL, SLO::CommandObject* pCommandObject)
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

		ROOTCALL CreateSwapChain(SLO::GL* pGL, SLO::CommandObject* pCommandObject)
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

		ROOTCALL CreateDescriptorHeaps(SLO::GL* pGL, SLO::DescriptorHeap* pDescriptorHeap)
		{
			auto* device = pGL->d3dDevice.Get();

			pDescriptorHeap->rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			pDescriptorHeap->dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			pDescriptorHeap->cbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
			rtvHeapDesc.NumDescriptors = Global::SWAP_CHAIN_BUFFER_COUNT;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			rtvHeapDesc.NodeMask = 0;
			ThrowIfFailed(device->CreateDescriptorHeap(
				&rtvHeapDesc, IID_PPV_ARGS(pDescriptorHeap->rtvHeap.GetAddressOf())));

			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			dsvHeapDesc.NodeMask = 0;
			ThrowIfFailed(device->CreateDescriptorHeap(
				&dsvHeapDesc, IID_PPV_ARGS(pDescriptorHeap->dsvHeap.GetAddressOf())));

			D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.NumDescriptors = Global::MAX_TEXTURE_COUNT;
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&pDescriptorHeap->srvHeap)));
		}

		ROOTCALL CreateBlur(BlurFilter* pBlurFilter, SLO::GL* pGL, SLO::DescriptorHeap* pDescriptorHeap)
		{
			pBlurFilter->OnInit(pGL->d3dDevice.Get(), pGL->clientWidth, pGL->clientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);

			pBlurFilter->BuildDescriptors(
				CD3DX12_CPU_DESCRIPTOR_HANDLE(pDescriptorHeap->srvHeap->GetCPUDescriptorHandleForHeapStart(),
					pDescriptorHeap->srvCount, pDescriptorHeap->cbvSrvUavDescriptorSize),
				CD3DX12_GPU_DESCRIPTOR_HANDLE(pDescriptorHeap->srvHeap->GetGPUDescriptorHandleForHeapStart(),
					pDescriptorHeap->srvCount, pDescriptorHeap->cbvSrvUavDescriptorSize),
				pDescriptorHeap->cbvSrvUavDescriptorSize, pDescriptorHeap->srvCount);
		}

		ROOTCALL CreateEtcs(GameTimer* pGameTimer, Camera* pCamera)
		{
			pGameTimer->Reset();
			pCamera->SetPosition(0.0f, 2.0f, -15.0f);
		}
	};

	struct GBuild
	{
		ROOTCALL BuildRootSignature(SLO::GL* pGL, SLO::RootSignature* pRootSignature)
		{
			//
			// default
			//
			{
				CD3DX12_DESCRIPTOR_RANGE texTable;
				texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

				// Root parameter can be a table, root descriptor or root constants.
				CD3DX12_ROOT_PARAMETER slotsRootParameter[4];

				// Perfomance TIP: Order from most frequent to least frequent.
				slotsRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
				slotsRootParameter[1].InitAsConstantBufferView(0);
				slotsRootParameter[2].InitAsConstantBufferView(1);
				slotsRootParameter[3].InitAsConstantBufferView(2);

				auto staticSamplers = GetStaticSamplers();

				// A root signature is an array of root parameters.
				CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotsRootParameter,
					(UINT)staticSamplers.size(), staticSamplers.data(),
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

				// create a root signature with a single SLOst which points to a descriptor range consisting of a single constant buffer
				ComPtr<ID3DBlob> serializedRootSig = nullptr;
				ComPtr<ID3DBlob> errorBlob = nullptr;
				HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
					serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

				if (errorBlob != nullptr)
				{
					::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				}
				ThrowIfFailed(hr);

				ThrowIfFailed(pGL->d3dDevice->CreateRootSignature(
					0,
					serializedRootSig->GetBufferPointer(),
					serializedRootSig->GetBufferSize(),
					IID_PPV_ARGS(pRootSignature->rootSignature.GetAddressOf())));
			}

			//
			// postprocess
			//
			{
				CD3DX12_DESCRIPTOR_RANGE srvTable;
				srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
			
				CD3DX12_DESCRIPTOR_RANGE uavTable;
				uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
			
				// Root parameter can be a table, root descriptor or root constants.
				CD3DX12_ROOT_PARAMETER slotRootParameter[3];
			
				// Perfomance TIP: Order from most frequent to least frequent.
				slotRootParameter[0].InitAsConstants(12, 0);
				slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);
				slotRootParameter[2].InitAsDescriptorTable(1, &uavTable);
			
				// A root signature is an array of root parameters.
				CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
					0, nullptr,
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			
				// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
				ComPtr<ID3DBlob> serializedRootSig = nullptr;
				ComPtr<ID3DBlob> errorBlob = nullptr;
				HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
					serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
			
				if (errorBlob != nullptr)
				{
					::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				}
				ThrowIfFailed(hr);
			
				ThrowIfFailed(pGL->d3dDevice->CreateRootSignature(
					0,
					serializedRootSig->GetBufferPointer(),
					serializedRootSig->GetBufferSize(),
					IID_PPV_ARGS(pRootSignature->postProcessRootSignature.GetAddressOf())));
			}
		}

		ROOTCALL BuildFrameResources(SLO::GL* pGL, SLO::ResourceManager* pResourceManager, Waves* pWaves)
		{
			auto* device = pGL->d3dDevice.Get();

			for (int i = 0; i < Global::FRAME_RESOURCE_COUNT; ++i)
			{
				auto frame = std::make_unique<SLO::FrameResource>();

				ThrowIfFailed(device->CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					IID_PPV_ARGS(frame->CmdListAlloc.GetAddressOf())));

				frame->PassCB = std::make_unique<UploadBuffer<SLO::PassConstants>>(device, Global::MAIN_PASS_COUNT, true);
				frame->MaterialCB = std::make_unique<UploadBuffer<SLO::MaterialConstants>>(device, Global::MAX_MATERIAL_COUNT, true);
				frame->ObjectCB = std::make_unique<UploadBuffer<SLO::ObjectConstants>>(device, Global::MAX_OBJECT_COUNT, true);

				frame->WavesVB = std::make_unique<UploadBuffer<SLO::Vertex>>(device, pWaves->VertexCount(), false);

				pResourceManager->frameResources.emplace_back(std::move(frame));
			}
		}

		ROOTCALL BuildInputLayer(SLO::PSOManager* pPSOManager)
		{
			pPSOManager->inputLayout =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			pPSOManager->treeSpriteInputLayout =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};
		}

		ROOTCALL BuildPSOs(SLO::GL* pGL, SLO::PSOManager* pPSOManager,
			SLO::RootSignature* pRootSignature,
			SLO::ShaderManager* pShaderManager)
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

			//
			// PSO for opaque objects.
			//
			ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
			opaquePsoDesc.InputLayout = { pPSOManager->inputLayout.data(), (UINT)pPSOManager->inputLayout.size() };
			opaquePsoDesc.pRootSignature = pRootSignature->rootSignature.Get();
			opaquePsoDesc.VS =
			{
				reinterpret_cast<BYTE*>(pShaderManager->shaders["standardVS"]->GetBufferPointer()),
				pShaderManager->shaders["standardVS"]->GetBufferSize()
			};
			opaquePsoDesc.PS =
			{
				reinterpret_cast<BYTE*>(pShaderManager->shaders["opaquePS"]->GetBufferPointer()),
				pShaderManager->shaders["opaquePS"]->GetBufferSize()
			};
			opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			opaquePsoDesc.SampleMask = UINT_MAX;
			opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			opaquePsoDesc.NumRenderTargets = 1;
			opaquePsoDesc.RTVFormats[0] = pGL->backBufferFormat;
			opaquePsoDesc.SampleDesc.Count = pGL->Msaa4xState ? 4 : 1;
			opaquePsoDesc.SampleDesc.Quality = pGL->Msaa4xState ? (pGL->Msaa4xQuality - 1) : 0;
			opaquePsoDesc.DSVFormat = pGL->depthStencilFormat;
			ThrowIfFailed(pGL->d3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&pPSOManager->PSOs[(int)SLO::RenderLayer::Opaque])));

			//
			// PSO for transparent objects
			//

			D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

			D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
			transparencyBlendDesc.BlendEnable = true;
			transparencyBlendDesc.LogicOpEnable = false;
			transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
			transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
			transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
			transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
			transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
			ThrowIfFailed(pGL->d3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&pPSOManager->PSOs[(int)SLO::RenderLayer::Transparent])));

			//
			// PSO for marking stencil mirrors.
			//

			CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
			mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0;

			D3D12_DEPTH_STENCIL_DESC mirrorDSS;
			mirrorDSS.DepthEnable = true;
			mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			mirrorDSS.StencilEnable = true;
			mirrorDSS.StencilReadMask = 0xff;
			mirrorDSS.StencilWriteMask = 0xff;

			mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
			mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

			// We are not rendering backfacing polygons, so these settings do not matter.
			mirrorDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			mirrorDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			mirrorDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
			mirrorDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorsPsoDesc = opaquePsoDesc;
			markMirrorsPsoDesc.BlendState = mirrorBlendState;
			markMirrorsPsoDesc.DepthStencilState = mirrorDSS;
			ThrowIfFailed(pGL->d3dDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&pPSOManager->PSOs[(int)SLO::RenderLayer::Mirrors])));

			//
			// PSO for stencil reflections.
			//

			D3D12_DEPTH_STENCIL_DESC reflectionsDSS;
			reflectionsDSS.DepthEnable = true;
			reflectionsDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			reflectionsDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			reflectionsDSS.StencilEnable = true;
			reflectionsDSS.StencilReadMask = 0xff;
			reflectionsDSS.StencilWriteMask = 0xff;

			reflectionsDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			reflectionsDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			reflectionsDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			reflectionsDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

			// We are not rendering backfacing polygons, so these settings do not matter.
			reflectionsDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			reflectionsDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			reflectionsDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			reflectionsDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC drawReflectionsPsoDesc = opaquePsoDesc;
			drawReflectionsPsoDesc.DepthStencilState = reflectionsDSS;
			drawReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
			drawReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;
			ThrowIfFailed(pGL->d3dDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&pPSOManager->PSOs[(int)SLO::RenderLayer::Reflected])));

			//
			// PSO for shadow objects
			//

			// We are going to draw shadows with transparency, so base it off the transparency description.
			D3D12_DEPTH_STENCIL_DESC shadowDSS;
			shadowDSS.DepthEnable = true;
			shadowDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			shadowDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			shadowDSS.StencilEnable = true;
			shadowDSS.StencilReadMask = 0xff;
			shadowDSS.StencilWriteMask = 0xff;

			shadowDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			shadowDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			shadowDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
			shadowDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

			// We are not rendering backfacing polygons, so these settings do not matter.
			shadowDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			shadowDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			shadowDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
			shadowDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = transparentPsoDesc;
			shadowPsoDesc.DepthStencilState = shadowDSS;
			ThrowIfFailed(pGL->d3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&pPSOManager->PSOs[(int)SLO::RenderLayer::Shadow])));

			//
			// PSO for alpha tested objects
			//

			D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
			alphaTestedPsoDesc.PS =
			{
				reinterpret_cast<BYTE*>(pShaderManager->shaders["alphaTestedPS"]->GetBufferPointer()),
				pShaderManager->shaders["alphaTestedPS"]->GetBufferSize()
			};
			alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			ThrowIfFailed(pGL->d3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&pPSOManager->PSOs[(int)SLO::RenderLayer::AlphaTested])));
			
			//
			// PSO for tree sprites
			//
			D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
			treeSpritePsoDesc.VS =
			{
				reinterpret_cast<BYTE*>(pShaderManager->shaders["treeSpriteVS"]->GetBufferPointer()),
				pShaderManager->shaders["treeSpriteVS"]->GetBufferSize()
			};
			treeSpritePsoDesc.GS =
			{
				reinterpret_cast<BYTE*>(pShaderManager->shaders["treeSpriteGS"]->GetBufferPointer()),
				pShaderManager->shaders["treeSpriteGS"]->GetBufferSize()
			};
			treeSpritePsoDesc.PS =
			{
				reinterpret_cast<BYTE*>(pShaderManager->shaders["treeSpritePS"]->GetBufferPointer()),
				pShaderManager->shaders["treeSpritePS"]->GetBufferSize()
			};
			treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			treeSpritePsoDesc.InputLayout = { pPSOManager->treeSpriteInputLayout.data(), (UINT)pPSOManager->treeSpriteInputLayout.size() };
			treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

			ThrowIfFailed(pGL->d3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&pPSOManager->PSOs[(int)SLO::RenderLayer::AlphaTestedTreeSprites])));

			//
			// PSO for horizontal blur
			//
			D3D12_COMPUTE_PIPELINE_STATE_DESC horzBlurPSO = {};
			horzBlurPSO.pRootSignature = pRootSignature->postProcessRootSignature.Get();
			horzBlurPSO.CS =
			{
				reinterpret_cast<BYTE*>(pShaderManager->shaders["horzBlurCS"]->GetBufferPointer()),
				pShaderManager->shaders["horzBlurCS"]->GetBufferSize()
			};
			horzBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			ThrowIfFailed(pGL->d3dDevice->CreateComputePipelineState(&horzBlurPSO, IID_PPV_ARGS(&pPSOManager->PSOs[(int)SLO::RenderLayer::HorzBlur])));
			
			//
			// PSO for vertical blur
			//
			D3D12_COMPUTE_PIPELINE_STATE_DESC vertBlurPSO = {};
			vertBlurPSO.pRootSignature = pRootSignature->postProcessRootSignature.Get();
			vertBlurPSO.CS =
			{
				reinterpret_cast<BYTE*>(pShaderManager->shaders["vertBlurCS"]->GetBufferPointer()),
				pShaderManager->shaders["vertBlurCS"]->GetBufferSize()
			};
			vertBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			ThrowIfFailed(pGL->d3dDevice->CreateComputePipelineState(&vertBlurPSO, IID_PPV_ARGS(&pPSOManager->PSOs[(int)SLO::RenderLayer::VertBlur])));
		}

		ROOTCALL BuildMeshes(SLO::GeometryManager* pGeometryManager)
		{
			SLP2G::GDelayCall::CreateMesh(pGeometryManager, "skullGeo", L"Models\\skull.txt");
			SLP2G::GDelayCall::CreateMesh(pGeometryManager, "carGeo", L"Models\\car.txt");
		}

		ROOTCALL BuildTextures(SLO::TextureManager* pTextureManager)
		{
			SLP2G::GDelayCall::CreateTexture(pTextureManager, "bricksTex", L"Textures/bricks3.dds");
			SLP2G::GDelayCall::CreateTexture(pTextureManager, "checkboardTex", L"Textures/checkboard.dds");
			SLP2G::GDelayCall::CreateTexture(pTextureManager, "iceTex", L"Textures/ice.dds");
			SLP2G::GDelayCall::CreateTexture(pTextureManager, "white1x1Tex", L"Textures/white1x1.dds");
			SLP2G::GDelayCall::CreateTexture(pTextureManager, "grassTex", L"Textures/grass.dds");
			SLP2G::GDelayCall::CreateTexture(pTextureManager, "waterTex", L"Textures/water1.dds");
			SLP2G::GDelayCall::CreateTexture(pTextureManager, "fenceTex", L"Textures/WireFence.dds");
			SLP2G::GDelayCall::CreateTextureArray(pTextureManager, "treeArrayTex", L"Textures/treeArray2.dds");
		}

		ROOTCALL BuildActors(SLO::ActorCollection* pActorCollection, SLO::GeometryManager* pGeometryManager,
			SLO::MaterialManager* pMaterialManager, SLO::RenderItemManager* pRenderItemManager)
		{
#define GEOGET(x) pGeometryManager->geometries[x].get()
#define ACTORGET(x) pActorCollection->actors[x].get()
#define MATGET(x) pMaterialManager->materials[x].get()

			auto* waterGeo = GEOGET("waterGeo");
			auto* landGeo = GEOGET("landGeo");
			auto* boxGeo = GEOGET("boxGeo");
			auto* treeSpritesGeo = GEOGET("treeSpritesGeo");
			auto* roomGeo = GEOGET("roomGeo");
			auto* skullGeo = GEOGET("skullGeo"); 

			int waterId, landId, boxId, treeSpriteId, roomId, skullId;
			SLP2G::GActor::CreateActor(pActorCollection, waterGeo, waterId);
			SLP2G::GActor::CreateActor(pActorCollection, landGeo, landId);
			SLP2G::GActor::CreateActor(pActorCollection, boxGeo, boxId);
			SLP2G::GActor::CreateActor(pActorCollection, treeSpritesGeo, treeSpriteId);
			//SLP2G::GActor::CreateActor(pActorCollection, roomGeo, roomId);
			//SLP2G::GActor::CreateActor(pActorCollection, skullGeo, skullId);

			auto* waterAct			= ACTORGET(waterId);
			XMStoreFloat4x4(&waterAct->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
			auto* landAct			= ACTORGET(landId);
			XMStoreFloat4x4(&landAct->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
			auto* boxAct			= ACTORGET(boxId);
			XMStoreFloat4x4(&boxAct->World, XMMatrixTranslation(3.0f, 2.0f, -9.0f));
			auto* treeSpritesAct	= ACTORGET(treeSpriteId);
			//auto* roomAct			= ACTORGET(roomId);
			//auto* skullAct		= ACTORGET(skullId);
			
			auto* grassMat			= MATGET("grass");
			auto* waterMat			= MATGET("water");
			auto* wirefenceMat		= MATGET("wirefence");
			auto* treeSpritesMat	= MATGET("treeSprites");
			auto* bricksMat			= MATGET("bricks");
			auto* checkertileMat	= MATGET("checkertile");
			auto* icemirrorMat		= MATGET("icemirror");
			auto* skullMat			= MATGET("skullMat");
			auto* shadowMat			= MATGET("shadowMat");

			SLP2G::GRenderer::CreateRenderItem(waterAct, waterMat, pRenderItemManager, SLO::RenderLayer::Transparent);
			pRenderItemManager->wavesRitem = pRenderItemManager->allritems[0].get();

			SLP2G::GRenderer::CreateRenderItem(landAct, grassMat, pRenderItemManager, SLO::RenderLayer::Opaque);
			SLP2G::GRenderer::CreateRenderItem(boxAct, wirefenceMat, pRenderItemManager, SLO::RenderLayer::AlphaTested);
			SLP2G::GRenderer::CreateRenderItem(treeSpritesAct, treeSpritesMat, pRenderItemManager, 
				SLO::RenderLayer::AlphaTestedTreeSprites, 0, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			//SLP2G::GRenderer::CreateRenderItem(roomAct, checkertileMat, pRenderItemManager, SLO::RenderLayer::Opaque, 0);
			//SLP2G::GRenderer::CreateRenderItem(roomAct, bricksMat, pRenderItemManager, SLO::RenderLayer::Opaque, 1);
			//SLP2G::GRenderer::CreateRenderItem(skullAct, skullMat, pRenderItemManager, SLO::RenderLayer::Opaque, 0);
			//SLP2G::GRenderer::CreateRenderItem(skullAct, skullMat, pRenderItemManager, SLO::RenderLayer::Reflected, 0);
			//SLP2G::GRenderer::CreateRenderItem(skullAct, shadowMat, pRenderItemManager, SLO::RenderLayer::Shadow, 0);
			//SLP2G::GRenderer::CreateRenderItem(roomAct, icemirrorMat, pRenderItemManager, SLO::RenderLayer::Mirrors, 2);
			//SLP2G::GRenderer::CreateRenderItem(roomAct, icemirrorMat, pRenderItemManager, SLO::RenderLayer::Transparent, 2);
		}

		ROOTCALL BuildShaders(SLO::ShaderManager* pShaderManager)
		{
			const D3D_SHADER_MACRO defines[] =
			{
				"FOG", "1",
				NULL, NULL
			};

			const D3D_SHADER_MACRO alphaTestDefines[] =
			{
				"FOG", "1",
				"ALPHA_TEST", "1",
				NULL, NULL
			};

			pShaderManager->shaders.emplace("standardVS", 
				d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0"));
			pShaderManager->shaders.emplace("opaquePS", 
				d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_0"));
			pShaderManager->shaders.emplace("alphaTestedPS", 
				d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_0"));

			pShaderManager->shaders.emplace("treeSpriteVS", 
				d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_0"));
			pShaderManager->shaders.emplace("treeSpriteGS", 
				d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_0"));
			pShaderManager->shaders.emplace("treeSpritePS", 
				d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_0"));

			pShaderManager->shaders.emplace("horzBlurCS",
				d3dUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "HorzBlurCS", "cs_5_0"));
			pShaderManager->shaders.emplace("vertBlurCS",
				d3dUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "VertBlurCS", "cs_5_0"));
		}

		ROOTCALL BuildMaterials(SLO::TextureManager* pTextureManager, SLO::MaterialManager* pMaterialManager)
		{
			using Material = SLO::Material;

			int matIndexer = 0;

			auto bricks = std::make_unique<Material>();
			bricks->Name = "bricks";
			bricks->MatCBIndex = matIndexer++;
			bricks->DiffuseTex = pTextureManager->textures["bricksTex"].get();
			bricks->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			bricks->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
			bricks->Roughness = 0.25f;

			auto checkertile = std::make_unique<Material>();
			checkertile->Name = "checkertile";
			checkertile->MatCBIndex = matIndexer++;
			checkertile->DiffuseTex = pTextureManager->textures["checkboardTex"].get();
			checkertile->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			checkertile->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
			checkertile->Roughness = 0.3f;

			auto icemirror = std::make_unique<Material>();
			icemirror->Name = "icemirror";
			icemirror->MatCBIndex = matIndexer++;
			icemirror->DiffuseTex = pTextureManager->textures["iceTex"].get();
			icemirror->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
			icemirror->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
			icemirror->Roughness = 0.5f;

			auto skullMat = std::make_unique<Material>();
			skullMat->Name = "skullMat";
			skullMat->MatCBIndex = matIndexer++;
			skullMat->DiffuseTex = pTextureManager->textures["white1x1Tex"].get();
			skullMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			skullMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
			skullMat->Roughness = 0.3f;

			auto shadowMat = std::make_unique<Material>();
			shadowMat->Name = "shadowMat";
			shadowMat->MatCBIndex = matIndexer++;
			shadowMat->DiffuseTex = pTextureManager->textures["white1x1Tex"].get();
			shadowMat->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
			shadowMat->FresnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
			shadowMat->Roughness = 0.0f;

			auto grass = std::make_unique<Material>();
			grass->Name = "grass";
			grass->MatCBIndex = matIndexer++;
			grass->DiffuseTex = pTextureManager->textures["grassTex"].get();
			grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
			grass->Roughness = 0.125f;

			// This is not a good water material definition, but we do not have all the rendering
			// tools we need (transparency, environment reflection), so we fake it for now.
			auto water = std::make_unique<Material>();
			water->Name = "water";
			water->MatCBIndex = matIndexer++;
			water->DiffuseTex = pTextureManager->textures["waterTex"].get();
			water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
			water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
			water->Roughness = 0.0f;

			auto wirefence = std::make_unique<Material>();
			wirefence->Name = "wirefence";
			wirefence->MatCBIndex = matIndexer++;
			wirefence->DiffuseTex = pTextureManager->textures["fenceTex"].get();
			wirefence->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			wirefence->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
			wirefence->Roughness = 0.25f;

			auto treeSprites = std::make_unique<Material>();
			treeSprites->Name = "treeSprites";
			treeSprites->MatCBIndex = matIndexer++;
			treeSprites->DiffuseTex = pTextureManager->textures["treeArrayTex"].get();
			treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
			treeSprites->Roughness = 0.125f;

			pMaterialManager->materials["bricks"] = std::move(bricks);
			pMaterialManager->materials["checkertile"] = std::move(checkertile);
			pMaterialManager->materials["icemirror"] = std::move(icemirror);
			pMaterialManager->materials["skullMat"] = std::move(skullMat);
			pMaterialManager->materials["shadowMat"] = std::move(shadowMat);
			pMaterialManager->materials["grass"] = std::move(grass);
			pMaterialManager->materials["water"] = std::move(water);
			pMaterialManager->materials["wirefence"] = std::move(wirefence);
			pMaterialManager->materials["treeSprites"] = std::move(treeSprites);
		}

	};

	struct GAnimate
	{
		ROOTCALL AnimateMaterials(SLO::MaterialManager* pMaterialManager, GameTimer* pGameTimer)
		{
			// Scroll the water material texture coordinates.
			auto waterMat = pMaterialManager->materials["water"].get();

			float& tu = waterMat->MatTransform(3, 0);
			float& tv = waterMat->MatTransform(3, 1);
			
			float dt = pGameTimer->DeltaTime();
			tu += 0.1f * dt;
			tv += 0.02f * dt;

			if (tu >= 1.0f)
				tu -= 1.0f;

			if (tv >= 1.0f)
				tv -= 1.0f;

			waterMat->MatTransform(3, 0) = tu;
			waterMat->MatTransform(3, 1) = tv;

			// Material has changed, so need to update cbuffer.
			waterMat->NumFramesDirty = Global::FRAME_RESOURCE_COUNT;
		}

	};

	struct GUpdate
	{
		ROOTCALL UpdateTimer(GameTimer* pGameTimer)
		{
			pGameTimer->Tick();
		}

		ROOTCALL UpdateFrame(SLO::ResourceManager* pResourceManager, SLO::CommandObject* pCommandObject)
		{
			// Cycle through the circular frame resource array.
			pResourceManager->currFrameResourceIndex = (pResourceManager->currFrameResourceIndex + 1) % Global::FRAME_RESOURCE_COUNT;
			pResourceManager->currFrameResource = pResourceManager->frameResources[pResourceManager->currFrameResourceIndex].get();

			// Has the GPU finished processing the commands of the current frame resource?
			// If not, wait until the GPU has completed commands up to this fence point.
			if (pResourceManager->currFrameResource->Fence != 0 && pCommandObject->fence->GetCompletedValue() < pResourceManager->currFrameResource->Fence)
			{
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, FALSE, EVENT_ALL_ACCESS);
				ThrowIfFailed(pCommandObject->fence->SetEventOnCompletion(pResourceManager->currFrameResource->Fence, eventHandle));
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}
		}

		ROOTCALL UpdateObjectCBs(SLO::ResourceManager* pResourceManager, SLO::ActorCollection* pActorCollection)
		{
			auto currObjectCB = pResourceManager->currFrameResource->ObjectCB.get();
			for (auto& e : pActorCollection->actors)
			{
				// Only update the cbuffer data if the constants have changed.  
				// This needs to be tracked per frame resource.
				if (e->NumFramesDirty > 0)
				{
					XMMATRIX world = XMLoadFloat4x4(&e->World);
					XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

					SLO::ObjectConstants objConstants;
					XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
					XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

					currObjectCB->CopyData(e->ObjCBIndex, objConstants);

					// Next FrameResource need to be updated too.
					e->NumFramesDirty--;
				}
			}
		}

		ROOTCALL UpdateMaterialCBs(SLO::ResourceManager* pResourceManager, SLO::MaterialManager* pMaterialManager)
		{
			auto currMaterialCB = pResourceManager->currFrameResource->MaterialCB.get();
			for (auto& e : pMaterialManager->materials)
			{
				// Only update the cbuffer data if the constants have changed.  If the cbuffer
				// data changes, it needs to be updated for each FrameResource.
				SLO::Material* mat = e.second.get();
				if (mat->NumFramesDirty > 0)
				{
					XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

					SLO::MaterialConstants matConstants;
					matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
					matConstants.FresnelR0 = mat->FresnelR0;
					matConstants.Roughness = mat->Roughness;
					XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

					currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

					// Next FrameResource need to be updated too.
					mat->NumFramesDirty--;
				}
			}
		}

		ROOTCALL UpdateMainPassCB(Camera* pCamera, SLO::ResourceManager* pResourceManager, SLO::GL* pGL, GameTimer* pGameTimer)
		{
			XMMATRIX view = pCamera->GetView();
			XMMATRIX proj = pCamera->GetProj();
			//XMMATRIX proj = pCamera->GetOrtho();

			XMMATRIX viewProj = XMMatrixMultiply(view, proj);
			XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
			XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
			XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

			auto& mainPass = pResourceManager->mainPassCB;
			XMStoreFloat4x4(&mainPass.View, XMMatrixTranspose(view));
			XMStoreFloat4x4(&mainPass.InvView, XMMatrixTranspose(invView));
			XMStoreFloat4x4(&mainPass.Proj, XMMatrixTranspose(proj));
			XMStoreFloat4x4(&mainPass.InvProj, XMMatrixTranspose(invProj));
			XMStoreFloat4x4(&mainPass.ViewProj, XMMatrixTranspose(viewProj));
			XMStoreFloat4x4(&mainPass.InvViewProj, XMMatrixTranspose(invViewProj));
			mainPass.EyePosW = pCamera->GetPosition3f();
			mainPass.RenderTargetSize = XMFLOAT2((float)pGL->clientWidth, (float)pGL->clientHeight);
			mainPass.InvRenderTargetSize = XMFLOAT2(1.0f / pGL->clientWidth, 1.0f / pGL->clientHeight);
			mainPass.NearZ = 1.0f;
			mainPass.FarZ = 1000.0f;
			mainPass.TotalTime = pGameTimer->TotalTime();
			mainPass.DeltaTime = pGameTimer->DeltaTime();
			mainPass.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
			mainPass.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
			mainPass.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
			mainPass.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
			mainPass.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
			mainPass.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
			mainPass.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

			// Main pass stored in index 2
			auto currPassCB = pResourceManager->currFrameResource->PassCB.get();
			currPassCB->CopyData(0, mainPass);
		}

		ROOTCALL UpdateReflectedPassCB(SLO::ResourceManager* pResourceManager)
		{
			pResourceManager->reflectedPassCB = pResourceManager->mainPassCB;

			XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
			XMMATRIX R = XMMatrixReflect(mirrorPlane);

			// Reflect the lighting.
			for (int i = 0; i < 3; ++i)
			{
				XMVECTOR lightDir = XMLoadFloat3(&pResourceManager->mainPassCB.Lights[i].Direction);
				XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
				XMStoreFloat3(&pResourceManager->reflectedPassCB.Lights[i].Direction, reflectedLightDir);
			}

			// Reflected pass stored in index 1
			auto currPassCB = pResourceManager->currFrameResource->PassCB.get();
			currPassCB->CopyData(1, pResourceManager->reflectedPassCB);
		}

		ROOTCALL UpdateWaves(GameTimer* pGameTimer, Waves* pWaves, SLO::ResourceManager* pResourceManager, 
			SLO::RenderItemManager* pRenderItemManager)
		{
			if (nullptr == pRenderItemManager->wavesRitem)
				return;

			// Every quarter second, generate a random wave.
			static float t_base = 0.0f;
			if ((pGameTimer->TotalTime() - t_base) >= 0.25f)
			{
				t_base += 0.25f;

				int i = MathHelper::Rand(4, pWaves->RowCount() - 5);
				int j = MathHelper::Rand(4, pWaves->ColumnCount() - 5);

				float r = MathHelper::RandF(0.2f, 0.5f);

				pWaves->Disturb(i, j, r);
			}

			// Update the wave simulation.
			pWaves->Update(pGameTimer->DeltaTime());

			// Update the wave vertex buffer with the new solution.
			auto currWavesVB = pResourceManager->currFrameResource->WavesVB.get();
			for (int i = 0; i < pWaves->VertexCount(); ++i)
			{
				SLO::Vertex v;

				v.Pos = pWaves->Position(i);
				v.Normal = pWaves->Normal(i);

				// Derive tex-coords from position by 
				// mapping [-w/2,w/2] --> [0,1]
				v.TexC.x = 0.5f + v.Pos.x / pWaves->Width();
				v.TexC.y = 0.5f - v.Pos.z / pWaves->Depth();

				currWavesVB->CopyData(i, v);
			}

			// Set the dynamic VB of the wave renderitem to the current frame VB.
			pRenderItemManager->wavesRitem->Act->Geo->VertexBufferGPU = currWavesVB->Resource();
		}

	};

	struct GTimer
	{
		FASTCALL Restart(GameTimer* pGameTimer)
		{
			pGameTimer->Start();
		}

		FASTCALL Pause(GameTimer* pGameTimer)
		{
			pGameTimer->Stop();
		}
	};

	struct GInput
	{
		ROOTCALL OnMouseDown(SLO::Mouse* pMouse, int x, int y, SLO::GL* pGL)
		{
			pMouse->lastMousePos.x = x;
			pMouse->lastMousePos.y = y;

			SetCapture(pGL->mainWnd);
		}

		ROOTCALL OnMouseUp(int x, int y)
		{
			ReleaseCapture();
		}

		ROOTCALL OnMouseMove(WPARAM btnState, int x, int y, SLO::Mouse* pMouse, Camera* pCamera)
		{
			if ((btnState & MK_LBUTTON) != 0)
			{
				// Make each pixel correspond to a quarter of a degree.
				float dx = XMConvertToRadians(0.25f * static_cast<float>(x - pMouse->lastMousePos.x));
				float dy = XMConvertToRadians(0.25f * static_cast<float>(y - pMouse->lastMousePos.y));

			}
			else if ((btnState & MK_RBUTTON) != 0)
			{
				// Make each pixel correspond to 0.005 unit in the scene.
				float dx = 0.005f * static_cast<float>(x - pMouse->lastMousePos.x);
				float dy = 0.005f * static_cast<float>(y - pMouse->lastMousePos.y);

				pCamera->Pitch(dy);
				pCamera->RotateY(dx); 
				//pCamera->Move(-dx, dy, 0.0f);
			}

			pMouse->lastMousePos.x = x;
			pMouse->lastMousePos.y = y;
		}

		ROOTCALL OnKeyboardInput(GameTimer* pGameTimer, Camera* pCamera)
		{
			//if (GetAsyncKeyState('1') & 0x8000)
			//	mIsWireframe = true;
			//else
			//	mIsWireframe = false;

			const float dt = pGameTimer->DeltaTime();

			if (GetAsyncKeyState('W') & 0x8000)
				pCamera->Walk(10.0f * dt);

			if (GetAsyncKeyState('S') & 0x8000)
				pCamera->Walk(-10.0f * dt);

			if (GetAsyncKeyState('A') & 0x8000)
				pCamera->Strafe(-10.0f * dt);

			if (GetAsyncKeyState('D') & 0x8000)
				pCamera->Strafe(10.0f * dt);

			pCamera->UpdateViewMatrix();
		}

	};

	struct GRender
	{
		ROOTCALL SetWindow(SLO::GL* pGL, int width, int height)
		{
			pGL->clientWidth = width;
			pGL->clientHeight = height;
		}

		ROOTCALL ResetBlur(BlurFilter* pBlurFilter, int width, int height)
		{
			pBlurFilter->OnResize(width, height);
		}

		ROOTCALL ResetChainBuffer(SLO::GL* pGL, SLO::DescriptorHeap* pDescriptorHeap)
		{
			// Release the previous resources we will be recreating.
			for (int i = 0; i < Global::SWAP_CHAIN_BUFFER_COUNT; ++i)
				pGL->swapChainBuffer[i].Reset();

			// Resize the swap chain.
			ThrowIfFailed(pGL->swapChain->ResizeBuffers(
				Global::SWAP_CHAIN_BUFFER_COUNT,
				pGL->clientWidth, pGL->clientHeight,
				pGL->backBufferFormat,
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

			pGL->currBackBuffer = 0;

			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(pDescriptorHeap->rtvHeap->GetCPUDescriptorHandleForHeapStart());
			for (UINT i = 0; i < Global::SWAP_CHAIN_BUFFER_COUNT; i++)
			{
				ThrowIfFailed(pGL->swapChain->GetBuffer(i, IID_PPV_ARGS(&pGL->swapChainBuffer[i])));
				pGL->d3dDevice->CreateRenderTargetView(pGL->swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
				rtvHeapHandle.Offset(1, pDescriptorHeap->rtvDescriptorSize);
			}
		}

		ROOTCALL ResetDepthStencilView(SLO::GL* pGL, SLO::DescriptorHeap* pDescriptorHeap, SLO::CommandObject* pCommandObject)
		{
			pGL->depthStencilBuffer.Reset();

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
			pGL->d3dDevice->CreateDepthStencilView(pGL->depthStencilBuffer.Get(), &dsvDesc, DepthStencilView(pDescriptorHeap));

			// Transition the resource from its initial state to be used as a depth buffer.
			pCommandObject->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pGL->depthStencilBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
		}

		ROOTCALL ResetViewport(SLO::GL* pGL, Camera* pCamera)
		{
			// Update the viewport transform to cover the client area.
			auto& viewport = pGL->screenViewport;
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = static_cast<float>(pGL->clientWidth);
			viewport.Height = static_cast<float>(pGL->clientHeight);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			pGL->scissorRect = { 0, 0, pGL->clientWidth, pGL->clientHeight };

			pCamera->SetLens(pCamera->GetFovY(), pCamera->GetAspect(), Global::SCREEN_NEAR, Global::SCREEN_DEPTH);
			//pCamera->SetOrtho(viewport.Width, viewport.Height, 1.0f, 1000.0f);
		}

		ROOTCALL Begin(SLO::GL* pGL, SLO::CommandObject* pCommandObject)
		{
			pCommandObject->commandList->RSSetViewports(1, &(pGL->screenViewport));
			pCommandObject->commandList->RSSetScissorRects(1, &(pGL->scissorRect));

			// Indicate a state transition on the resource usage.
			pCommandObject->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(pGL),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		}

		ROOTCALL SetRenderTarget(SLO::DescriptorHeap* pDescriptorHeap, SLO::GL* pGL, SLO::CommandObject* pCommandObject,
			SLO::ResourceManager* pResourceManager)
		{
			const auto& currentBackBufferView = CurrentBackBufferView(pDescriptorHeap, pGL);
			const auto& depthstencilView = DepthStencilView(pDescriptorHeap);

			auto& cmdList = pCommandObject->commandList;

			// Clear the back buffer and depth buffer.
			cmdList->ClearRenderTargetView(currentBackBufferView, (float*)&(pResourceManager->mainPassCB.FogColor), 0, nullptr);
			cmdList->ClearDepthStencilView(depthstencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthstencilView);
		}

		ROOTCALL SetDescriptorHeaps(SLO::DescriptorHeap* pDescriptorHeap, SLO::CommandObject* pCommandObject)
		{
			ID3D12DescriptorHeap* descriptorHeaps[] = { pDescriptorHeap->srvHeap.Get() };
			pCommandObject->commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		}

		ROOTCALL SetRootSignature(SLO::CommandObject* pCommandObject, SLO::RootSignature* pRootSignature)
		{
			pCommandObject->commandList->SetGraphicsRootSignature(pRootSignature->rootSignature.Get());
		}

		ROOTCALL SetPassConstantsBuffer(SLO::ResourceManager* pResourceManager, SLO::CommandObject* pCommandObject, int passIdx)
		{
			auto* passCB = pResourceManager->currFrameResource->PassCB->Resource();
			pCommandObject->commandList->SetGraphicsRootConstantBufferView(Global::PASSCB_PARAMETER_INDEX, passCB->GetGPUVirtualAddress() + (passIdx - 1) * passCBByteSize);
		}

		ROOTCALL SetStencil(SLO::CommandObject* pCommandObject, int refVal)
		{
			pCommandObject->commandList->OMSetStencilRef(refVal);
		}

		ROOTCALL Draw(SLO::CommandObject* pCommandObject, SLO::PSOManager* pPSOManager, SLO::ResourceManager* pResourceManager,
			SLO::RenderItemManager* pRenderItemManager, SLO::DescriptorHeap* pDescriptorHeap, SLO::RenderLayer layer)
		{
			auto& cmdList = pCommandObject->commandList;
			cmdList->SetPipelineState(pPSOManager->PSOs[(int)layer].Get());

			auto* objectCB = pResourceManager->currFrameResource->ObjectCB->Resource();
			auto* matCB = pResourceManager->currFrameResource->MaterialCB->Resource();
			
			auto& renderBundle = pRenderItemManager->ritemLayer[(int)layer];
			
			// For each render item...
			for (size_t i = 0; i < renderBundle.items.size(); ++i)
			{
				auto ri = renderBundle.items[i];
				auto* actor = ri->Act;
			
				cmdList->IASetVertexBuffers(0, 1, &actor->Geo->VertexBufferView());
				cmdList->IASetIndexBuffer(&actor->Geo->IndexBufferView());
				cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
				
				CD3DX12_GPU_DESCRIPTOR_HANDLE tex(pDescriptorHeap->srvHeap->GetGPUDescriptorHandleForHeapStart());
				tex.Offset(ri->Mat->DiffuseTex->srvOffset, pDescriptorHeap->cbvSrvUavDescriptorSize);
				
				D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + actor->ObjCBIndex * objCBByteSize;
				D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;
				
				cmdList->SetGraphicsRootDescriptorTable(0, tex);
				cmdList->SetGraphicsRootConstantBufferView(Global::OBJECTCB_PARAMETER_INDEX, objCBAddress);
				cmdList->SetGraphicsRootConstantBufferView(Global::MATCB_PARAMETER_INDEX, matCBAddress);
				
				cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
			}
		}

		ROOTCALL End(SLO::GL* pGL, SLO::CommandObject* pCommandObject)
		{
			// Indicate a state transition on the resource usage.
			pCommandObject->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(pGL),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		}

		ROOTCALL PostProcess(BlurFilter* pBlurFilter, SLO::CommandObject* pCommandObject, SLO::RootSignature* pRootSignature,
			SLO::PSOManager* pPSOManager, SLO::GL* pGL)
		{
			auto* commandList = pCommandObject->commandList.Get();

			pBlurFilter->Execute(commandList, pRootSignature->postProcessRootSignature.Get(),
				pPSOManager->PSOs[(int)SLO::RenderLayer::HorzBlur].Get(),
				pPSOManager->PSOs[(int)SLO::RenderLayer::VertBlur].Get(),
				CurrentBackBuffer(pGL), 4);

			// Prepare to copy blurred output to the back buffer.
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(pGL),
				D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

			commandList->CopyResource(CurrentBackBuffer(pGL), pBlurFilter->Output());

			// Transition to PRESENT state.
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(pGL),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));

		}

		ROOTCALL SwapChainBuffer(SLO::GL* pGL)
		{
			// Swap the back and front buffers
			ThrowIfFailed(pGL->swapChain->Present(0, 0));
			pGL->currBackBuffer = (pGL->currBackBuffer + 1) % Global::SWAP_CHAIN_BUFFER_COUNT;
		}

	};

	struct GPostProcess
	{
		ROOTCALL ProcessMeshes(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager)
		{
			auto& queue = pGeometryManager->waitqueue;
			if (queue.empty())
				return;

			while (!queue.empty())
			{
				auto item = queue.front();
				queue.pop();

				SLP2G::GMesh::BuildTxtGeometry(pGL, pCommandObject, pGeometryManager, item.first, item.second);
			}
		}

		ROOTCALL ProcessTextures(SLO::TextureManager* pTextureManager, SLO::GL* pGL, SLO::CommandObject* pCommandObject, 
			SLO::DescriptorHeap* pDescriptorHeap)
		{
			using Texture = SLO::Texture;

			auto& queue = pTextureManager->waitqueue;
			if (queue.empty())
				return;

			auto& map = pTextureManager->textures;
			auto& device = pGL->d3dDevice;
			auto& cmdList = pCommandObject->commandList;
			while (!queue.empty())
			{
				auto item = queue.front();
				queue.pop();

				auto tex = std::make_unique<Texture>();
				tex->name = item.name;
				tex->filename = item.filename;
				ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device.Get(), cmdList.Get(),
					tex->filename.c_str(), tex->resource, tex->uploadHeap));

				auto res = tex->resource;
				tex->srvOffset = pDescriptorHeap->srvCount++;

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = item.srvDesc;
				srvDesc.Format = res->GetDesc().Format;
				srvDesc.Texture2DArray.ArraySize = res->GetDesc().DepthOrArraySize;

				CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor = SrvHeapHandle(pDescriptorHeap);
				hDescriptor.Offset(tex->srvOffset, pDescriptorHeap->cbvSrvUavDescriptorSize);

				device->CreateShaderResourceView(res.Get(), &srvDesc, hDescriptor);

				map[tex->name] = std::move(tex);
			}
		}
	};

}
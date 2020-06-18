#pragma once
#include "pch.h"
#include "SLO.h"
#include "GlobalVar.h"
#include "d3dx12.h"
#include "d3dUtil.h"
#include "Pures.h"

namespace SLP
{
#define ROOTCALL __forceinline static void

	using namespace Microsoft::WRL;
	using namespace DirectX;

	static const UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(SLO::ObjectConstants));
	static const UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(SLO::PassConstants));
	static const UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(SLO::MaterialConstants));

	struct GConstruct
	{
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

		ROOTCALL BuildRootSignature(SLO::GL* pGL, SLO::RootSignature* pRootSignature)
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

		ROOTCALL LoadTextures(SLO::TextureManager* pTextureManager, SLO::GL* pGL, SLO::CommandObject* pCommandObject,
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
				tex->name = item.first;
				tex->filename = item.second;
				ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device.Get(), cmdList.Get(),
					tex->filename.c_str(), tex->resource, tex->uploadHeap));

				auto res = tex->resource;
				tex->srvOffset = pDescriptorHeap->srvCount++;

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Format = res->GetDesc().Format;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = -1;

				CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor = SrvHeapHandle(pDescriptorHeap);
				hDescriptor.Offset(tex->srvOffset, pDescriptorHeap->cbvSrvUavDescriptorSize);

				device->CreateShaderResourceView(res.Get(), &srvDesc, hDescriptor);

				map[tex->name] = std::move(tex);
			}
		}

		// TODO: 런타임 추가
		ROOTCALL BuildMaterials(SLO::TextureManager* pTextureManager, SLO::MaterialManager* pMaterialManager)
		{
			using Material = SLO::Material;

			auto bricks = std::make_unique<Material>();
			bricks->Name = "bricks";
			bricks->MatCBIndex = 0;
			bricks->DiffuseTex = pTextureManager->textures["bricks"].get();
			bricks->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			bricks->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
			bricks->Roughness = 0.25f;

			auto checkertile = std::make_unique<Material>();
			checkertile->Name = "checkertile";
			checkertile->MatCBIndex = 1;
			checkertile->DiffuseTex = pTextureManager->textures["checkboardTex"].get();
			checkertile->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			checkertile->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
			checkertile->Roughness = 0.3f;

			auto icemirror = std::make_unique<Material>();
			icemirror->Name = "icemirror";
			icemirror->MatCBIndex = 2;
			icemirror->DiffuseTex = pTextureManager->textures["iceTex"].get();
			icemirror->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
			icemirror->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
			icemirror->Roughness = 0.5f;

			auto skullMat = std::make_unique<Material>();
			skullMat->Name = "skullMat";
			skullMat->MatCBIndex = 3;
			skullMat->DiffuseTex = pTextureManager->textures["white1x1Tex"].get();
			skullMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			skullMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
			skullMat->Roughness = 0.3f;

			auto shadowMat = std::make_unique<Material>();
			shadowMat->Name = "shadowMat";
			shadowMat->MatCBIndex = 4;
			shadowMat->DiffuseTex = pTextureManager->textures["white1x1Tex"].get();
			shadowMat->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
			shadowMat->FresnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
			shadowMat->Roughness = 0.0f;

			pMaterialManager->materials["bricks"] = std::move(bricks);
			pMaterialManager->materials["checkertile"] = std::move(checkertile);
			pMaterialManager->materials["icemirror"] = std::move(icemirror);
			pMaterialManager->materials["skullMat"] = std::move(skullMat);
			pMaterialManager->materials["shadowMat"] = std::move(shadowMat);
		}

		static void AddRenderItem(SLO::Material* pMaterial, SLO::MeshGeometry* pMeshGeometry,
			SLO::RenderItemManager* pRenderItemManager, SLO::RenderLayer layer, int submesh)
		{
			using RenderItem = SLO::RenderItem;

			auto ritem = std::make_unique<RenderItem>();
			ritem->World = MathHelper::Identity4x4();
			ritem->TexTransform = MathHelper::Identity4x4();
			ritem->ObjCBIndex = pRenderItemManager->itemCount++;
			ritem->Mat = pMaterial;
			ritem->Geo = pMeshGeometry;
			ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			ritem->IndexCount = ritem->Geo->Submeshes[submesh].IndexCount;
			ritem->StartIndexLocation = ritem->Geo->Submeshes[submesh].StartIndexLocation;
			ritem->BaseVertexLocation = ritem->Geo->Submeshes[submesh].BaseVertexLocation;
			pRenderItemManager->ritemLayer[(int)layer].items.push_back(ritem.get());
			pRenderItemManager->allritems.emplace_back(ritem);
		}

		ROOTCALL BuildFrameResources(SLO::GL* pGL, SLO::ResourceManager* pResourceManager)
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

				pResourceManager->frameResources.emplace_back(frame);
			}
		}

		ROOTCALL BuildPSOs(SLO::GL* pGL, SLO::PSOManager* pPSOManager,
			SLO::RootSignature* pRootSignature,
			SLO::ShaderManager* pShaderManager)
		{
			auto& inputLayer = pPSOManager->inputLayout;
			inputLayer =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

			//
			// PSO for opaque objects.
			//
			ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
			opaquePsoDesc.InputLayout = { inputLayer.data(), (UINT)inputLayer.size() };
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
		}

		ROOTCALL InitializeAdvanced(SLO::GL* pGL, SLO::CommandObject* pCommandObject,
			SLO::DescriptorHeap* pDescriptorHeap,
			SLO::RootSignature* pRootSignature,
			SLO::TextureManager* pTextureManager,
			SLO::ShaderManager* pShaderManager,
			SLO::GeometryManager* pGeometryManager,
			SLO::MaterialManager* pMaterialManager,
			SLO::RenderItemManager* pRenderItemManager,
			SLO::ResourceManager* pResourceManager,
			SLO::PSOManager* pPSOManager,
			Camera* pCamera)
		{
			// Reset the command list to prep for initialization commands.
			auto* allocator = pCommandObject->directCmdListAlloc.Get();
			ThrowIfFailed(pCommandObject->commandList->Reset(allocator, nullptr));

			pTextureManager->Add("bricksTex", L"Textures/bricks3.dds");
			pTextureManager->Add("checkboardTex", L"Textures/checkboard.dds");
			pTextureManager->Add("iceTex", L"Textures/ice.dds");
			pTextureManager->Add("white1x1Tex", L"Textures/white1x1.dds");
			LoadTextures(pTextureManager, pGL, pCommandObject, pDescriptorHeap);

			BuildRootSignature(pGL, pRootSignature);
			//BuildDescriptorHeaps();

			//BuildShadersAndInputLayout();
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

			pShaderManager->shaders.emplace("standardVS", d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0"));
			pShaderManager->shaders.emplace("opaquePS", d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "vs_5_0"));
			pShaderManager->shaders.emplace("alphaTestedPS", d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "vs_5_0"));

			GGeometry::BuildRoomGeometry(pGL, pCommandObject, pGeometryManager);
			GGeometry::BuildTxtGeometry(pGL, pCommandObject, pGeometryManager, L"Models/skull.txt");
			BuildMaterials(pTextureManager, pMaterialManager);

			//BuildRenderItems();
			auto* matbricks = pMaterialManager->materials["bricks"].get();
			auto* matcheckertile = pMaterialManager->materials["checkertile"].get();
			auto* maticemirror = pMaterialManager->materials["icemirror"].get();
			auto* matskullMat = pMaterialManager->materials["skullMat"].get();
			auto* matshadowMat = pMaterialManager->materials["shadowMat"].get();
			auto* roomGeo = pGeometryManager->geometries["roomGeo"].get();
			auto* skullGeo = pGeometryManager->geometries["skullGeo"].get();
			AddRenderItem(matcheckertile, roomGeo, pRenderItemManager, SLO::RenderLayer::Opaque, 0);
			AddRenderItem(matbricks, roomGeo, pRenderItemManager, SLO::RenderLayer::Opaque, 1);
			AddRenderItem(matskullMat, skullGeo, pRenderItemManager, SLO::RenderLayer::Opaque, 0);
			AddRenderItem(matskullMat, skullGeo, pRenderItemManager, SLO::RenderLayer::Reflected, 0);
			AddRenderItem(matshadowMat, skullGeo, pRenderItemManager, SLO::RenderLayer::Shadow, 0);
			AddRenderItem(maticemirror, roomGeo, pRenderItemManager, SLO::RenderLayer::Mirrors, 2);
			AddRenderItem(maticemirror, roomGeo, pRenderItemManager, SLO::RenderLayer::Transparent, 2);

			BuildFrameResources(pGL, pResourceManager);
			BuildPSOs(pGL, pPSOManager, pRootSignature, pShaderManager);

			pCamera->SetPosition(0.0f, 2.0f, -15.0f);
		}
	};

	struct GCommander
	{
		ROOTCALL ResetDirectly(SLO::CommandObject* pCommandObject)
		{
			// Reset the command list to prep for initialization commands.
			ThrowIfFailed(pCommandObject->commandList->Reset(pCommandObject->directCmdListAlloc.Get(), nullptr));
		}

		ROOTCALL Signal(SLO::CommandObject* pCommandObject)
		{
			// Advance the fence value to mark commands up to this fence point.
			pCommandObject->currentFence++;

			// Notify the fence when the GPU completes commands up to this fence point.
			pCommandObject->commandQueue->Signal(pCommandObject->fence.Get(), pCommandObject->currentFence);
		}

		ROOTCALL Wait(SLO::CommandObject* pCommandObject)
		{
			// Wait until the GPU has completed commands up to this fence point.
			if (pCommandObject->fence->GetCompletedValue() < pCommandObject->currentFence)
			{
				HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

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
				HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
				ThrowIfFailed(pCommandObject->fence->SetEventOnCompletion(pResourceManager->currFrameResource->Fence, eventHandle));
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}
		}

		ROOTCALL UpdateObjectCBs(SLO::ResourceManager* pResourceManager, SLO::RenderItemManager* pRenderItemManager)
		{
			auto currObjectCB = pResourceManager->currFrameResource->ObjectCB.get();
			for (auto& e : pRenderItemManager->allritems)
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
			XMMATRIX proj = pCamera->GetOrtho();

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

				//mCamera.Pitch(dy);
				//mCamera.RotateY(dx); 
				pCamera->Move(-dx, dy, 0.0f);
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
		ROOTCALL Resize(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::DescriptorHeap* pDescriptorHeap, Camera* pCamera)
		{
			assert(pGL->d3dDevice);
			assert(pGL->swapChain);
			assert(pCommandObject->directCmdListAlloc);

			// Flush before changing any resources.
			GCommander::Flush(pCommandObject);

			auto* allocator = pCommandObject->directCmdListAlloc.Get();
			ThrowIfFailed(pCommandObject->commandList->Reset(allocator, nullptr));

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

			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(pDescriptorHeap->rtvHeap->GetCPUDescriptorHandleForHeapStart());
			for (UINT i = 0; i < Global::SWAP_CHAIN_BUFFER_COUNT; i++)
			{
				ThrowIfFailed(pGL->swapChain->GetBuffer(i, IID_PPV_ARGS(&pGL->swapChainBuffer[i])));
				pGL->d3dDevice->CreateRenderTargetView(pGL->swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
				rtvHeapHandle.Offset(1, pDescriptorHeap->rtvDescriptorSize);
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
			pGL->d3dDevice->CreateDepthStencilView(pGL->depthStencilBuffer.Get(), &dsvDesc, DepthStencilView(pDescriptorHeap));

			// Transition the resource from its initial state to be used as a depth buffer.
			pCommandObject->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pGL->depthStencilBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

			// Execute the resize commands.
			GCommander::Execute(pCommandObject);

			// Wait until resize is complete.
			GCommander::Flush(pCommandObject);

			// Update the viewport transform to cover the client area.
			auto& viewport = pGL->screenViewport;
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = static_cast<float>(pGL->clientWidth);
			viewport.Height = static_cast<float>(pGL->clientHeight);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			pGL->scissorRect = { 0, 0, pGL->clientWidth, pGL->clientHeight };

			pCamera->SetOrtho(viewport.Width, viewport.Height, 1.0f, 1000.0f);
		}

		static void DrawRenderItems(SLO::FrameResource* pFrameResource, SLO::RenderBundle* pRenderBundle, 
			SLO::CommandObject* pCommandObject,
			SLO::DescriptorHeap* pDescriptorHeap)
		{
			auto objectCB = pFrameResource->ObjectCB->Resource();
			auto matCB = pFrameResource->MaterialCB->Resource();

			auto& cmdList = pCommandObject->commandList;

			// For each render item...
			for (size_t i = 0; i < pRenderBundle->items.size(); ++i)
			{
				auto ri = pRenderBundle->items[i];

				cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
				cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
				cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

				CD3DX12_GPU_DESCRIPTOR_HANDLE tex(pDescriptorHeap->srvHeap->GetGPUDescriptorHandleForHeapStart());
				tex.Offset(ri->Mat->DiffuseTex->srvOffset, pDescriptorHeap->cbvSrvUavDescriptorSize);

				D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
				D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

				cmdList->SetGraphicsRootDescriptorTable(0, tex);
				cmdList->SetGraphicsRootConstantBufferView(Global::OBJECTCB_PARAMETER_INDEX, objCBAddress);
				cmdList->SetGraphicsRootConstantBufferView(Global::MATCB_PARAMETER_INDEX, matCBAddress);

				cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
			}
		}

		ROOTCALL Draw(SLO::ResourceManager* pResourceManager, SLO::CommandObject* pCommandObject, 
			SLO::PSOManager* pPSOManager,
			SLO::GL* pGL,
			SLO::DescriptorHeap* pDescriptorHeap,
			SLO::RootSignature* pRootSignature,
			SLO::RenderItemManager* pRenderItemManager)
		{
			auto& curFrame = pResourceManager->currFrameResource;
			auto& cmdListAlloc = curFrame->CmdListAlloc;
			auto& mCommandList = pCommandObject->commandList;
			auto& mRitemLayer = pRenderItemManager->ritemLayer;

			// Reuse the memory associated with command recording.
			// We can only reset when the associated command lists have finished execution on the GPU.
			ThrowIfFailed(cmdListAlloc->Reset());

			// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
			// Reusing the command list reuses memory.
			ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), pPSOManager->PSOs[(int)SLO::RenderLayer::Opaque].Get()));

			mCommandList->RSSetViewports(1, &(pGL->screenViewport));
			mCommandList->RSSetScissorRects(1, &(pGL->scissorRect));

			// Indicate a state transition on the resource usage.
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(pGL),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

			auto currentBackBufferView = BackBufferView(pDescriptorHeap, pGL->currBackBuffer);
			auto depthstencilView = DepthStencilView(pDescriptorHeap);

			// Clear the back buffer and depth buffer.
			mCommandList->ClearRenderTargetView(currentBackBufferView, (float*)&(pResourceManager->mainPassCB.FogColor), 0, nullptr);
			mCommandList->ClearDepthStencilView(depthstencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			mCommandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthstencilView);

			ID3D12DescriptorHeap* descriptorHeaps[] = { pDescriptorHeap->srvHeap.Get() };
			mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

			mCommandList->SetGraphicsRootSignature(pRootSignature->rootSignature.Get());


			// Draw opaque items--floors, walls, skull.
			auto passCB = curFrame->PassCB->Resource();
			mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
			mCommandList->SetPipelineState(pPSOManager->PSOs[(int)SLO::RenderLayer::Opaque].Get());
			DrawRenderItems(curFrame, &mRitemLayer[(int)SLO::RenderLayer::Opaque], pCommandObject, pDescriptorHeap);

			// Mark the visible mirror pixels in the stencil buffer with the value 1
			mCommandList->OMSetStencilRef(1);
			mCommandList->SetPipelineState(pPSOManager->PSOs[(int)SLO::RenderLayer::Mirrors].Get());
			DrawRenderItems(curFrame, &mRitemLayer[(int)SLO::RenderLayer::Mirrors], pCommandObject, pDescriptorHeap);

			// Draw the reflection into the mirror only (only for pixels where the stencil buffer is 1).
			// Note that we must supply a different per-pass constant buffer--one with the lights reflected.
			mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress() + 1 * passCBByteSize);
			mCommandList->SetPipelineState(pPSOManager->PSOs[(int)SLO::RenderLayer::Reflected].Get());
			DrawRenderItems(curFrame, &mRitemLayer[(int)SLO::RenderLayer::Reflected], pCommandObject, pDescriptorHeap);

			// Restore main pass constants and stencil ref.
			mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
			mCommandList->OMSetStencilRef(0);

			// Draw mirror with transparency so reflection blends through.
			mCommandList->SetPipelineState(pPSOManager->PSOs[(int)SLO::RenderLayer::Transparent].Get());
			DrawRenderItems(curFrame, &mRitemLayer[(int)SLO::RenderLayer::Transparent], pCommandObject, pDescriptorHeap);

			// Draw shadows
			mCommandList->SetPipelineState(pPSOManager->PSOs[(int)SLO::RenderLayer::Shadow].Get());
			DrawRenderItems(curFrame, &mRitemLayer[(int)SLO::RenderLayer::Shadow], pCommandObject, pDescriptorHeap);

			End(pGL, pCommandObject);

			GCommander::Execute(pCommandObject);

			SwapBuffer(pGL);

			GCommander::Signal(pCommandObject);
		}

		ROOTCALL Begin(SLO::FrameResource* pFrameResource, SLO::GL* pGL, SLO::CommandObject* pCommandObject)
		{
			// Reuse the memory associated with command recording.
			// We can only reset when the associated command lists have finished execution on the GPU.
			ThrowIfFailed(pFrameResource->CmdListAlloc->Reset());

			// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
			// Reusing the command list reuses memory.
			ThrowIfFailed(pCommandObject->commandList->Reset(pFrameResource->CmdListAlloc.Get(), nullptr));

			pCommandObject->commandList->RSSetViewports(1, &(pGL->screenViewport));
			pCommandObject->commandList->RSSetScissorRects(1, &(pGL->scissorRect));

			// Indicate a state transition on the resource usage.
			pCommandObject->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(pGL),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		}

		ROOTCALL SetRenderTarget(SLO::DescriptorHeap* pDescriptorHeap, SLO::GL* pGL, SLO::CommandObject* pCommandObject,
			SLO::ResourceManager* pResourceManager)
		{
			auto currentBackBufferView = BackBufferView(pDescriptorHeap, pGL->currBackBuffer);
			auto depthstencilView = DepthStencilView(pDescriptorHeap);

			// Clear the back buffer and depth buffer.
			pCommandObject->commandList->ClearRenderTargetView(currentBackBufferView, (float*)&(pResourceManager->mainPassCB.FogColor), 0, nullptr);
			pCommandObject->commandList->ClearDepthStencilView(depthstencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			pCommandObject->commandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthstencilView);
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

		ROOTCALL SetPassConstantsBuffer(SLO::FrameResource* pFrameResource, SLO::CommandObject* pCommandObject, int passIdx)
		{
			auto* passCB = pFrameResource->PassCB->Resource();
			pCommandObject->commandList->SetGraphicsRootConstantBufferView(Global::PASSCB_PARAMETER_INDEX, passCB->GetGPUVirtualAddress() + (passIdx - 1) * passCBByteSize);
		}

		ROOTCALL SetStencil(SLO::CommandObject* pCommandObject, int refVal)
		{
			pCommandObject->commandList->OMSetStencilRef(refVal);
		}

		ROOTCALL Render(SLO::CommandObject* pCommandObject, SLO::PSOManager* pPSOManager, SLO::FrameResource* pFrameResource,
			SLO::RenderItemManager* pRenderItemManager, SLO::DescriptorHeap* pDescriptorHeap, SLO::RenderLayer layer)
		{
			pCommandObject->commandList->SetPipelineState(pPSOManager->PSOs[(int)layer].Get());
			DrawRenderItems(pFrameResource, &pRenderItemManager->ritemLayer[(int)layer], pCommandObject, pDescriptorHeap);
		}

		ROOTCALL End(SLO::GL* pGL, SLO::CommandObject* pCommandObject)
		{
			// Indicate a state transition on the resource usage.
			pCommandObject->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(pGL),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		}

		ROOTCALL SwapBuffer(SLO::GL* pGL)
		{
			// Swap the back and front buffers
			ThrowIfFailed(pGL->swapChain->Present(0, 0));
			pGL->currBackBuffer = (pGL->currBackBuffer + 1) % Global::SWAP_CHAIN_BUFFER_COUNT;
		}
	};

	struct GGeometry
	{
		using Vertex = SLO::Vertex;
		using SubmeshGeometry = SLO::SubmeshGeometry;
		using MeshGeometry = SLO::MeshGeometry;

		static void BuildRoomGeometry(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager)
		{
			// Create and specify geometry.  For this sample we draw a floor
			// and a wall with a mirror on it.  We put the floor, wall, and
			// mirror geometry in one vertex buffer.
			//
			//   |--------------|
			//   |              |
			//   |----|----|----|
			//   |Wall|Mirr|Wall|
			//   |    | or |    |
			//   /--------------/
			//  /   Floor      /
			// /--------------/

			std::array<Vertex, 20> vertices =
			{
				// Floor: Observe we tile texture coordinates.
				Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
				Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
				Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
				Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

				// Wall: Observe we tile texture coordinates, and that we
				// leave a gap in the middle for the mirror.
				Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
				Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
				Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
				Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

				Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
				Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
				Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
				Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

				Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
				Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
				Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
				Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

				// Mirror
				Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
				Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
				Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
				Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
			};

			std::array<std::int16_t, 30> indices =
			{
				// Floor
				0, 1, 2,
				0, 2, 3,

				// Walls
				4, 5, 6,
				4, 6, 7,

				8, 9, 10,
				8, 10, 11,

				12, 13, 14,
				12, 14, 15,

				// Mirror
				16, 17, 18,
				16, 18, 19
			};

			SubmeshGeometry floorSubmesh;
			floorSubmesh.IndexCount = 6;
			floorSubmesh.StartIndexLocation = 0;
			floorSubmesh.BaseVertexLocation = 0;

			SubmeshGeometry wallSubmesh;
			wallSubmesh.IndexCount = 18;
			wallSubmesh.StartIndexLocation = 6;
			wallSubmesh.BaseVertexLocation = 0;

			SubmeshGeometry mirrorSubmesh;
			mirrorSubmesh.IndexCount = 6;
			mirrorSubmesh.StartIndexLocation = 24;
			mirrorSubmesh.BaseVertexLocation = 0;

			const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
			const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

			auto geo = std::make_unique<MeshGeometry>();
			geo->Name = "roomGeo";

			ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
			CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

			geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

			geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

			geo->VertexByteStride = sizeof(Vertex);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = DXGI_FORMAT_R16_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			geo->Submeshes.emplace_back(floorSubmesh);
			geo->Submeshes.emplace_back(wallSubmesh);
			geo->Submeshes.emplace_back(mirrorSubmesh);

			pGeometryManager->geometries[geo->Name] = std::move(geo);
		}

		static void BuildTxtGeometry(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager, const std::wstring& filename)
		{
			std::ifstream fin(filename);

			if (!fin)
			{
				wchar_t message[512];
				swprintf_s(message, L"%s not found.", filename.c_str());
				MessageBox(0, message, 0, 0);
				return;
			}

			UINT vcount = 0;
			UINT tcount = 0;
			std::string ignore;

			fin >> ignore >> vcount;
			fin >> ignore >> tcount;
			fin >> ignore >> ignore >> ignore >> ignore;

			std::vector<Vertex> vertices(vcount);
			for (UINT i = 0; i < vcount; ++i)
			{
				fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
				fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

				// Model does not have texture coordinates, so just zero them out.
				vertices[i].TexC = { 0.0f, 0.0f };
			}

			fin >> ignore;
			fin >> ignore;
			fin >> ignore;

			std::vector<std::int32_t> indices(3 * tcount);
			for (UINT i = 0; i < tcount; ++i)
			{
				fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
			}

			fin.close();

			//
			// Pack the indices of all the meshes into one index buffer.
			//

			const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

			const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

			auto geo = std::make_unique<MeshGeometry>();
			geo->Name = "skullGeo";

			ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
			CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

			geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

			geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

			geo->VertexByteStride = sizeof(Vertex);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = DXGI_FORMAT_R32_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)indices.size();
			submesh.StartIndexLocation = 0;
			submesh.BaseVertexLocation = 0;

			geo->Submeshes.emplace_back(submesh);

			pGeometryManager->geometries[geo->Name] = std::move(geo);
		}

	};
}
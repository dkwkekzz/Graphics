#pragma once
#include "pch.h"
#include "objects.h"

// This macro determines whether to detect if there is an HDR display and enable HDR10 output.
// Currently, with HDR display enabled, the pixel magnfication functionality is broken.
#define CONDITIONALLY_ENABLE_HDR_OUTPUT 1

// Uncomment this to enable experimental support for the new shader compiler, DXC.exe
//#define DXIL

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#include <agile.h>
#endif

#if defined(NTDDI_WIN10_RS2) && (NTDDI_VERSION >= NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_4.h>    // For WARP
#endif
#include <winreg.h>        // To read the registry

struct slpGraphics
{
    static void InitializeCommonState(sloGraphicsCommon* gcn, sloGraphicsCore* gce)
    {
        gcn->SamplerLinearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        gcn->SamplerLinearWrap = gcn->SamplerLinearWrapDesc.CreateDescriptor();

        gcn->SamplerAnisoWrapDesc.MaxAnisotropy = 4;
        gcn->SamplerAnisoWrap = gcn->SamplerAnisoWrapDesc.CreateDescriptor();

        gcn->SamplerShadowDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        gcn->SamplerShadowDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        gcn->SamplerShadowDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        gcn->SamplerShadow = gcn->SamplerShadowDesc.CreateDescriptor();

        gcn->SamplerLinearClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        gcn->SamplerLinearClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        gcn->SamplerLinearClamp = gcn->SamplerLinearClampDesc.CreateDescriptor();

        gcn->SamplerVolumeWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        gcn->SamplerVolumeWrap = gcn->SamplerVolumeWrapDesc.CreateDescriptor();

        gcn->SamplerPointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        gcn->SamplerPointClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        gcn->SamplerPointClamp = gcn->SamplerPointClampDesc.CreateDescriptor();

        gcn->SamplerLinearBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        gcn->SamplerLinearBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        gcn->SamplerLinearBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
        gcn->SamplerLinearBorder = gcn->SamplerLinearBorderDesc.CreateDescriptor();

        gcn->SamplerPointBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        gcn->SamplerPointBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        gcn->SamplerPointBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
        gcn->SamplerPointBorder = gcn->SamplerPointBorderDesc.CreateDescriptor();

        // Default rasterizer states
        gcn->RasterizerDefault.FillMode = D3D12_FILL_MODE_SOLID;
        gcn->RasterizerDefault.CullMode = D3D12_CULL_MODE_BACK;
        gcn->RasterizerDefault.FrontCounterClockwise = TRUE;
        gcn->RasterizerDefault.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        gcn->RasterizerDefault.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        gcn->RasterizerDefault.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        gcn->RasterizerDefault.DepthClipEnable = TRUE;
        gcn->RasterizerDefault.MultisampleEnable = FALSE;
        gcn->RasterizerDefault.AntialiasedLineEnable = FALSE;
        gcn->RasterizerDefault.ForcedSampleCount = 0;
        gcn->RasterizerDefault.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        gcn->RasterizerDefaultMsaa = gcn->RasterizerDefault;
        gcn->RasterizerDefaultMsaa.MultisampleEnable = TRUE;

        gcn->RasterizerDefaultCw = gcn->RasterizerDefault;
        gcn->RasterizerDefaultCw.FrontCounterClockwise = FALSE;

        gcn->RasterizerDefaultCwMsaa = gcn->RasterizerDefaultCw;
        gcn->RasterizerDefaultCwMsaa.MultisampleEnable = TRUE;

        gcn->RasterizerTwoSided = gcn->RasterizerDefault;
        gcn->RasterizerTwoSided.CullMode = D3D12_CULL_MODE_NONE;

        gcn->RasterizerTwoSidedMsaa = gcn->RasterizerTwoSided;
        gcn->RasterizerTwoSidedMsaa.MultisampleEnable = TRUE;

        // Shadows need their own rasterizer state so we can reverse the winding of faces
        gcn->RasterizerShadow = gcn->RasterizerDefault;
        //RasterizerShadow.CullMode = D3D12_CULL_FRONT;  // Hacked here rather than fixing the content
        gcn->RasterizerShadow.SlopeScaledDepthBias = -1.5f;
        gcn->RasterizerShadow.DepthBias = -100;

        gcn->RasterizerShadowTwoSided = gcn->RasterizerShadow;
        gcn->RasterizerShadowTwoSided.CullMode = D3D12_CULL_MODE_NONE;

        gcn->RasterizerShadowCW = gcn->RasterizerShadow;
        gcn->RasterizerShadowCW.FrontCounterClockwise = FALSE;

        gcn->DepthStateDisabled.DepthEnable = FALSE;
        gcn->DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        gcn->DepthStateDisabled.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        gcn->DepthStateDisabled.StencilEnable = FALSE;
        gcn->DepthStateDisabled.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        gcn->DepthStateDisabled.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        gcn->DepthStateDisabled.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        gcn->DepthStateDisabled.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        gcn->DepthStateDisabled.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        gcn->DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        gcn->DepthStateDisabled.BackFace = gcn->DepthStateDisabled.FrontFace;

        gcn->DepthStateReadWrite = gcn->DepthStateDisabled;
        gcn->DepthStateReadWrite.DepthEnable = TRUE;
        gcn->DepthStateReadWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        gcn->DepthStateReadWrite.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

        gcn->DepthStateReadOnly = gcn->DepthStateReadWrite;
        gcn->DepthStateReadOnly.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

        gcn->DepthStateReadOnlyReversed = gcn->DepthStateReadOnly;
        gcn->DepthStateReadOnlyReversed.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        gcn->DepthStateTestEqual = gcn->DepthStateReadOnly;
        gcn->DepthStateTestEqual.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

        D3D12_BLEND_DESC alphaBlend = {};
        alphaBlend.IndependentBlendEnable = FALSE;
        alphaBlend.RenderTarget[0].BlendEnable = FALSE;
        alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        alphaBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        alphaBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        alphaBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        alphaBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        alphaBlend.RenderTarget[0].RenderTargetWriteMask = 0;
        gcn->BlendNoColorWrite = alphaBlend;

        alphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        gcn->BlendDisable = alphaBlend;

        alphaBlend.RenderTarget[0].BlendEnable = TRUE;
        gcn->BlendTraditional = alphaBlend;

        alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        gcn->BlendPreMultiplied = alphaBlend;

        alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        gcn->BlendAdditive = alphaBlend;

        alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        gcn->BlendTraditionalAdditive = alphaBlend;

        gcn->DispatchIndirectCommandSignature[0].Dispatch();
        gcn->DispatchIndirectCommandSignature.Finalize();

        gcn->DrawIndirectCommandSignature[0].Draw();
        gcn->DrawIndirectCommandSignature.Finalize();

        //BitonicSort::Initialize();
    }

    static void InitializeBitonicSort()
    {
        s_DispatchArgs.Create(L"Bitonic sort dispatch args", 22 * 23 / 2, 12);

        s_RootSignature.Reset(4, 0);
        s_RootSignature[0].InitAsConstants(0, 2);
        s_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
        s_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        s_RootSignature[3].InitAsConstants(1, 2);
        s_RootSignature.Finalize(L"Bitonic Sort");

#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName.SetRootSignature(s_RootSignature); \
    ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize();

        CreatePSO(s_BitonicIndirectArgsCS, g_pBitonicIndirectArgsCS);
        CreatePSO(s_Bitonic32PreSortCS, g_pBitonic32PreSortCS);
        CreatePSO(s_Bitonic32InnerSortCS, g_pBitonic32InnerSortCS);
        CreatePSO(s_Bitonic32OuterSortCS, g_pBitonic32OuterSortCS);
        CreatePSO(s_Bitonic64PreSortCS, g_pBitonic64PreSortCS);
        CreatePSO(s_Bitonic64InnerSortCS, g_pBitonic64InnerSortCS);
        CreatePSO(s_Bitonic64OuterSortCS, g_pBitonic64OuterSortCS);

#undef CreatePSO
    }

    static void DestroyCommonState(void)
    {
        DispatchIndirectCommandSignature.Destroy();
        DrawIndirectCommandSignature.Destroy();

        BitonicSort::Shutdown();
    }

    static void DestroyBitonicSort(void)
    {
        s_DispatchArgs.Destroy();
    }

	static void Initialize(sloGraphicsCore* gce, sloFrameCounter* fc, sloGraphicsCommon* gcn, HWND mhWnd)
	{
        ASSERT(gce->mSwapChain1 == nullptr, "Graphics has already been initialized");

        Microsoft::WRL::ComPtr<ID3D12Device> pDevice;

#if _DEBUG
        Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
        if (SUCCEEDED(D3D12GetDebugInterface(MY_IID_PPV_ARGS(&debugInterface))))
            debugInterface->EnableDebugLayer();
        else
            Utility::Print("WARNING:  Unable to enable D3D12 debug validation layer\n");
#endif

        //EnableExperimentalShaderModels();

        // Obtain the DXGI factory
        Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
        ASSERT_SUCCEEDED(CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory)));

        // Create the D3D graphics device
        Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;

        static const bool bUseWarpDriver = false;

        if (!bUseWarpDriver)
        {
            SIZE_T MaxSize = 0;

            for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(Idx, &pAdapter); ++Idx)
            {
                DXGI_ADAPTER_DESC1 desc;
                pAdapter->GetDesc1(&desc);
                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    continue;

                if (desc.DedicatedVideoMemory > MaxSize&& SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, MY_IID_PPV_ARGS(&pDevice))))
                {
                    pAdapter->GetDesc1(&desc);
                    Utility::Printf(L"D3D12-capable hardware found:  %s (%u MB)\n", desc.Description, desc.DedicatedVideoMemory >> 20);
                    MaxSize = desc.DedicatedVideoMemory;
                }
            }

            if (MaxSize > 0)
                gce->mDevice = pDevice.Detach();
        }

        if (gce->mDevice == nullptr)
        {
            if (bUseWarpDriver)
                Utility::Print("WARP software adapter requested.  Initializing...\n");
            else
                Utility::Print("Failed to find a hardware adapter.  Falling back to WARP.\n");
            ASSERT_SUCCEEDED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter)));
            ASSERT_SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, MY_IID_PPV_ARGS(&pDevice)));
            gce->mDevice = pDevice.Detach();
        }
#ifndef RELEASE
        else
        {
            bool DeveloperModeEnabled = false;

            // Look in the Windows Registry to determine if Developer Mode is enabled
            HKEY hKey;
            LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &hKey);
            if (result == ERROR_SUCCESS)
            {
                DWORD keyValue, keySize = sizeof(DWORD);
                result = RegQueryValueEx(hKey, L"AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
                if (result == ERROR_SUCCESS && keyValue == 1)
                    DeveloperModeEnabled = true;
                RegCloseKey(hKey);
            }

            WARN_ONCE_IF_NOT(DeveloperModeEnabled, "Enable Developer Mode on Windows 10 to get consistent profiling results");

            // Prevent the GPU from overclocking or underclocking to get consistent timings
            if (DeveloperModeEnabled)
                gce->mDevice->SetStablePowerState(TRUE);
        }
#endif    

#if _DEBUG
        ID3D12InfoQueue* pInfoQueue = nullptr;
        if (SUCCEEDED(gce->mDevice->QueryInterface(MY_IID_PPV_ARGS(&pInfoQueue))))
        {
            // Suppress whole categories of messages
            //D3D12_MESSAGE_CATEGORY Categories[] = {};

            // Suppress messages based on their severity level
            D3D12_MESSAGE_SEVERITY Severities[] =
            {
                D3D12_MESSAGE_SEVERITY_INFO
            };

            // Suppress individual messages by their ID
            D3D12_MESSAGE_ID DenyIds[] =
            {
                // This occurs when there are uninitialized descriptors in a descriptor table, even when a
                // shader does not access the missing descriptors.  I find this is common when switching
                // shader permutations and not wanting to change much code to reorder resources.
                D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

                // Triggered when a shader does not export all color components of a render target, such as
                // when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
                D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

                // This occurs when a descriptor table is unbound even when a shader does not access the missing
                // descriptors.  This is common with a root signature shared between disparate shaders that
                // don't all need the same types of resources.
                D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

                // RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
                (D3D12_MESSAGE_ID)1008,
            };

            D3D12_INFO_QUEUE_FILTER NewFilter = {};
            //NewFilter.DenyList.NumCategories = _countof(Categories);
            //NewFilter.DenyList.pCategoryList = Categories;
            NewFilter.DenyList.NumSeverities = _countof(Severities);
            NewFilter.DenyList.pSeverityList = Severities;
            NewFilter.DenyList.NumIDs = _countof(DenyIds);
            NewFilter.DenyList.pIDList = DenyIds;

            pInfoQueue->PushStorageFilter(&NewFilter);
            pInfoQueue->Release();
        }
#endif

        // We like to do read-modify-write operations on UAVs during post processing.  To support that, we
        // need to either have the hardware do typed UAV loads of R11G11B10_FLOAT or we need to manually
        // decode an R32_UINT representation of the same buffer.  This code determines if we get the hardware
        // load support.
        D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};
        if (SUCCEEDED(gce->mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData))))
        {
            if (FeatureData.TypedUAVLoadAdditionalFormats)
            {
                D3D12_FEATURE_DATA_FORMAT_SUPPORT Support =
                {
                    DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE
                };

                if (SUCCEEDED(gce->mDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
                    (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
                {
                    gce->mbTypedUAVLoadSupport_R11G11B10_FLOAT = true;
                }

                Support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

                if (SUCCEEDED(gce->mDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
                    (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
                {
                    gce->mbTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
                }
            }
        }

        gce->mCommandManager.Create(gce->mDevice);

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = gce->mDisplayWidth;
        swapChainDesc.Height = gce->mDisplayHeight;
        swapChainDesc.Format = SWAP_CHAIN_FORMAT;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) // Win32
        ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(gce->mCommandManager.GetCommandQueue(), mhWnd, &swapChainDesc, nullptr, nullptr, &gce->mSwapChain1));
#else // UWP
        ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForCoreWindow(gce->mCommandManager.GetCommandQueue(), (IUnknown*)GameCore::gce->mwindow.Get(), &swapChainDesc, nullptr, &gce->mSwapChain1));
#endif

#if CONDITIONALLY_ENABLE_HDR_OUTPUT && defined(NTDDI_WIN10_RS2) && (NTDDI_VERSION >= NTDDI_WIN10_RS2)
        {
            IDXGISwapChain4* swapChain = (IDXGISwapChain4*)gce->mSwapChain1;
            ComPtr<IDXGIOutput> output;
            ComPtr<IDXGIOutput6> output6;
            DXGI_OUTPUT_DESC1 outputDesc;
            UINT colorSpaceSupport;

            // Query support for ST.2084 on the display and set the color space accordingly
            if (SUCCEEDED(swapChain->GetContainingOutput(&output)) &&
                SUCCEEDED(output.As(&output6)) &&
                SUCCEEDED(output6->GetDesc1(&outputDesc)) &&
                outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 &&
                SUCCEEDED(swapChain->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport)) &&
                (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) &&
                SUCCEEDED(swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))
            {
                gce->mbEnableHDROutput = true;
            }
        }
#endif

        for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        {
            ComPtr<ID3D12Resource> DisplayPlane;
            ASSERT_SUCCEEDED(gce->mSwapChain1->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane)));
            gce->mDisplayPlane[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
        }

        // Common state was moved to GraphicsCommon.*
        InitializeCommonState(gcn, gce);

        InitializeRootSignature();

        InitializePSOs();
	}

    static void InitializeRootSignature(sloGraphicsCore* gce, sloGraphicsCommon* gcn)
    {
        gce->mPresentRS.Reset(4, 2);
        gce->mPresentRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
        gce->mPresentRS[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
        gce->mPresentRS[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
        gce->mPresentRS[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        gce->mPresentRS.InitStaticSampler(0, gcn->SamplerLinearClampDesc);
        gce->mPresentRS.InitStaticSampler(1, gcn->SamplerPointClampDesc);
        gce->mPresentRS.Finalize(L"Present");
    }

    static void InitializePSOs(sloGraphicsCore* gce, sloGraphicsCommon* gcn)
    {
        // Initialize PSOs
        gce->mBlendUIPSO.SetRootSignature(gce->mPresentRS);
        gce->mBlendUIPSO.SetRasterizerState(gcn->RasterizerTwoSided);
        gce->mBlendUIPSO.SetBlendState(gcn->BlendPreMultiplied);
        gce->mBlendUIPSO.SetDepthStencilState(gcn->DepthStateDisabled);
        gce->mBlendUIPSO.SetSampleMask(0xFFFFFFFF);
        gce->mBlendUIPSO.SetInputLayout(0, nullptr);
        gce->mBlendUIPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        gce->mBlendUIPSO.SetVertexShader(gce->mpScreenQuadVS, sizeof(gce->mpScreenQuadVS));
        gce->mBlendUIPSO.SetPixelShader(gce->mpBufferCopyPS, sizeof(gce->mpBufferCopyPS));
        gce->mBlendUIPSO.SetRenderTargetFormat(SWAP_CHAIN_FORMAT, DXGI_FORMAT_UNKNOWN);
        gce->mBlendUIPSO.Finalize();

#define CreatePSO( ObjName, ShaderByteCode ) \
        ObjName = gce->mBlendUIPSO; \
        ObjName.SetBlendState( BlendDisable ); \
        ObjName.SetPixelShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
        ObjName.Finalize();

        CreatePSO(gce->PresentSDRPS, gce->mpPresentSDRPS);
        CreatePSO(gce->MagnifyPixelsPS, gce->mpMagnifyPixelsPS);
        CreatePSO(gce->BilinearUpsamplePS, gce->mpBilinearUpsamplePS);
        CreatePSO(gce->BicubicHorizontalUpsamplePS, gce->mpBicubicHorizontalUpsamplePS);
        CreatePSO(gce->BicubicVerticalUpsamplePS, gce->mpBicubicVerticalUpsamplePS);
        CreatePSO(gce->SharpeningUpsamplePS, gce->mpSharpeningUpsamplePS);

#undef CreatePSO

        gce->BicubicHorizontalUpsamplePS = gce->mBlendUIPSO;
        gce->BicubicHorizontalUpsamplePS.SetBlendState(BlendDisable);
        gce->BicubicHorizontalUpsamplePS.SetPixelShader(gcn->mpBicubicHorizontalUpsamplePS, sizeof(gce->mpBicubicHorizontalUpsamplePS));
        gce->BicubicHorizontalUpsamplePS.SetRenderTargetFormat(DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_UNKNOWN);
        gce->BicubicHorizontalUpsamplePS.Finalize();

        gce->PresentHDRPS = PresentSDRPS;
        gce->PresentHDRPS.SetPixelShader(gce->mpPresentHDRPS, sizeof(gce->mpPresentHDRPS));
        DXGI_FORMAT SWAP_CHAIN_FORMATs[2] = { DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM };
        gce->PresentHDRPS.SetRenderTargetFormats(2, SWAP_CHAIN_FORMATs, DXGI_FORMAT_UNKNOWN);
        gce->PresentHDRPS.Finalize();

        gce->mGenerateMipsRS.Reset(3, 1);
        gce->mGenerateMipsRS[0].InitAsConstants(0, 4);
        gce->mGenerateMipsRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
        gce->mGenerateMipsRS[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 4);
        gce->mGenerateMipsRS.InitStaticSampler(0, SamplerLinearClampDesc);
        gce->mGenerateMipsRS.Finalize(L"Generate Mips");

#define CreatePSO(ObjName, ShaderByteCode ) \
        ObjName.SetRootSignature(gce->mGenerateMipsRS); \
        ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
        ObjName.Finalize();

        CreatePSO(gce->mGenerateMipsLinearPSO[0], gce->mpGenerateMipsLinearCS);
        CreatePSO(gce->mGenerateMipsLinearPSO[1], gce->mpGenerateMipsLinearOddXCS);
        CreatePSO(gce->mGenerateMipsLinearPSO[2], gce->mpGenerateMipsLinearOddYCS);
        CreatePSO(gce->mGenerateMipsLinearPSO[3], gce->mpGenerateMipsLinearOddCS);
        CreatePSO(gce->mGenerateMipsGammaPSO[0], gce->mpGenerateMipsGammaCS);
        CreatePSO(gce->mGenerateMipsGammaPSO[1], gce->mpGenerateMipsGammaOddXCS);
        CreatePSO(gce->mGenerateMipsGammaPSO[2], gce->mpGenerateMipsGammaOddYCS);
        CreatePSO(gce->mGenerateMipsGammaPSO[3], gce->mpGenerateMipsGammaOddCS);

        gce->mPreDisplayBuffer.Create(L"PreDisplay Buffer", gce->mDisplayWidth, gce->mDisplayHeight, 1, SWAP_CHAIN_FORMAT);

        GpuTimeManager::Initialize(4096);
        SetNativeResolution();
        TemporalEffects::Initialize();
        PostEffects::Initialize();
        SSAO::Initialize();
        TextRenderer::Initialize();
        GraphRenderer::Initialize();
        ParticleEffects::Initialize(kMaxNativeWidth, kMaxNativeHeight);
    }
};
#pragma once
#include "pch.h"
#include "processes2G.h"

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

#include "../CompiledShaders/ScreenQuadVS.h"
#include "../CompiledShaders/BufferCopyPS.h"
#include "../CompiledShaders/PresentSDRPS.h"
#include "../CompiledShaders/PresentHDRPS.h"
#include "../CompiledShaders/MagnifyPixelsPS.h"
#include "../CompiledShaders/BilinearUpsamplePS.h"
#include "../CompiledShaders/BicubicHorizontalUpsamplePS.h"
#include "../CompiledShaders/BicubicVerticalUpsamplePS.h"
#include "../CompiledShaders/SharpeningUpsamplePS.h"
#include "../CompiledShaders/GenerateMipsLinearCS.h"
#include "../CompiledShaders/GenerateMipsLinearOddCS.h"
#include "../CompiledShaders/GenerateMipsLinearOddXCS.h"
#include "../CompiledShaders/GenerateMipsLinearOddYCS.h"
#include "../CompiledShaders/GenerateMipsGammaCS.h"
#include "../CompiledShaders/GenerateMipsGammaOddCS.h"
#include "../CompiledShaders/GenerateMipsGammaOddXCS.h"
#include "../CompiledShaders/GenerateMipsGammaOddYCS.h"
#include "../CompiledShaders/BitonicIndirectArgsCS.h"
#include "../CompiledShaders/Bitonic32PreSortCS.h"
#include "../CompiledShaders/Bitonic32InnerSortCS.h"
#include "../CompiledShaders/Bitonic32OuterSortCS.h"
#include "../CompiledShaders/Bitonic64PreSortCS.h"
#include "../CompiledShaders/Bitonic64InnerSortCS.h"
#include "../CompiledShaders/Bitonic64OuterSortCS.h"

struct slpGraphics
{
    static void InitializeCommonState(sloGraphicsCommon* pCommon)
    {
        pCommon->SamplerLinearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        pCommon->SamplerLinearWrap = pCommon->SamplerLinearWrapDesc.CreateDescriptor();

        pCommon->SamplerAnisoWrapDesc.MaxAnisotropy = 4;
        pCommon->SamplerAnisoWrap = pCommon->SamplerAnisoWrapDesc.CreateDescriptor();

        pCommon->SamplerShadowDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        pCommon->SamplerShadowDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        pCommon->SamplerShadowDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        pCommon->SamplerShadow = pCommon->SamplerShadowDesc.CreateDescriptor();

        pCommon->SamplerLinearClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        pCommon->SamplerLinearClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        pCommon->SamplerLinearClamp = pCommon->SamplerLinearClampDesc.CreateDescriptor();

        pCommon->SamplerVolumeWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        pCommon->SamplerVolumeWrap = pCommon->SamplerVolumeWrapDesc.CreateDescriptor();

        pCommon->SamplerPointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        pCommon->SamplerPointClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        pCommon->SamplerPointClamp = pCommon->SamplerPointClampDesc.CreateDescriptor();

        pCommon->SamplerLinearBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        pCommon->SamplerLinearBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        pCommon->SamplerLinearBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
        pCommon->SamplerLinearBorder = pCommon->SamplerLinearBorderDesc.CreateDescriptor();

        pCommon->SamplerPointBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        pCommon->SamplerPointBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        pCommon->SamplerPointBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
        pCommon->SamplerPointBorder = pCommon->SamplerPointBorderDesc.CreateDescriptor();

        // Default rasterizer states
        pCommon->RasterizerDefault.FillMode = D3D12_FILL_MODE_SOLID;
        pCommon->RasterizerDefault.CullMode = D3D12_CULL_MODE_BACK;
        pCommon->RasterizerDefault.FrontCounterClockwise = TRUE;
        pCommon->RasterizerDefault.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        pCommon->RasterizerDefault.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        pCommon->RasterizerDefault.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        pCommon->RasterizerDefault.DepthClipEnable = TRUE;
        pCommon->RasterizerDefault.MultisampleEnable = FALSE;
        pCommon->RasterizerDefault.AntialiasedLineEnable = FALSE;
        pCommon->RasterizerDefault.ForcedSampleCount = 0;
        pCommon->RasterizerDefault.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        pCommon->RasterizerDefaultMsaa = pCommon->RasterizerDefault;
        pCommon->RasterizerDefaultMsaa.MultisampleEnable = TRUE;

        pCommon->RasterizerDefaultCw = pCommon->RasterizerDefault;
        pCommon->RasterizerDefaultCw.FrontCounterClockwise = FALSE;

        pCommon->RasterizerDefaultCwMsaa = pCommon->RasterizerDefaultCw;
        pCommon->RasterizerDefaultCwMsaa.MultisampleEnable = TRUE;

        pCommon->RasterizerTwoSided = pCommon->RasterizerDefault;
        pCommon->RasterizerTwoSided.CullMode = D3D12_CULL_MODE_NONE;

        pCommon->RasterizerTwoSidedMsaa = pCommon->RasterizerTwoSided;
        pCommon->RasterizerTwoSidedMsaa.MultisampleEnable = TRUE;

        // Shadows need their own rasterizer state so we can reverse the winding of faces
        pCommon->RasterizerShadow = pCommon->RasterizerDefault;
        //RasterizerShadow.CullMode = D3D12_CULL_FRONT;  // Hacked here rather than fixing the content
        pCommon->RasterizerShadow.SlopeScaledDepthBias = -1.5f;
        pCommon->RasterizerShadow.DepthBias = -100;

        pCommon->RasterizerShadowTwoSided = pCommon->RasterizerShadow;
        pCommon->RasterizerShadowTwoSided.CullMode = D3D12_CULL_MODE_NONE;

        pCommon->RasterizerShadowCW = pCommon->RasterizerShadow;
        pCommon->RasterizerShadowCW.FrontCounterClockwise = FALSE;

        pCommon->DepthStateDisabled.DepthEnable = FALSE;
        pCommon->DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        pCommon->DepthStateDisabled.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pCommon->DepthStateDisabled.StencilEnable = FALSE;
        pCommon->DepthStateDisabled.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        pCommon->DepthStateDisabled.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        pCommon->DepthStateDisabled.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pCommon->DepthStateDisabled.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        pCommon->DepthStateDisabled.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        pCommon->DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        pCommon->DepthStateDisabled.BackFace = pCommon->DepthStateDisabled.FrontFace;

        pCommon->DepthStateReadWrite = pCommon->DepthStateDisabled;
        pCommon->DepthStateReadWrite.DepthEnable = TRUE;
        pCommon->DepthStateReadWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        pCommon->DepthStateReadWrite.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

        pCommon->DepthStateReadOnly = pCommon->DepthStateReadWrite;
        pCommon->DepthStateReadOnly.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

        pCommon->DepthStateReadOnlyReversed = pCommon->DepthStateReadOnly;
        pCommon->DepthStateReadOnlyReversed.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        pCommon->DepthStateTestEqual = pCommon->DepthStateReadOnly;
        pCommon->DepthStateTestEqual.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

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
        pCommon->BlendNoColorWrite = alphaBlend;

        alphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pCommon->BlendDisable = alphaBlend;

        alphaBlend.RenderTarget[0].BlendEnable = TRUE;
        pCommon->BlendTraditional = alphaBlend;

        alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        pCommon->BlendPreMultiplied = alphaBlend;

        alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        pCommon->BlendAdditive = alphaBlend;

        alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        pCommon->BlendTraditionalAdditive = alphaBlend;

        pCommon->DispatchIndirectCommandSignature[0].Dispatch();
        pCommon->DispatchIndirectCommandSignature.Finalize();

        pCommon->DrawIndirectCommandSignature[0].Draw();
        pCommon->DrawIndirectCommandSignature.Finalize();

        //BitonicSort::Initialize();
    }

    static void InitializeBitonicSort(sloBitonicSort* pBitonicSort)
    {
        pBitonicSort->s_DispatchArgs.Create(L"Bitonic sort dispatch args", 22 * 23 / 2, 12);

        pBitonicSort->s_RootSignature.Reset(4, 0);
        pBitonicSort->s_RootSignature[0].InitAsConstants(0, 2);
        pBitonicSort->s_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
        pBitonicSort->s_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        pBitonicSort->s_RootSignature[3].InitAsConstants(1, 2);
        pBitonicSort->s_RootSignature.Finalize(L"Bitonic Sort");

#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName.SetRootSignature(pBitonicSort->s_RootSignature); \
    ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize();

        CreatePSO(pBitonicSort->s_BitonicIndirectArgsCS, g_pBitonicIndirectArgsCS);
        CreatePSO(pBitonicSort->s_Bitonic32PreSortCS, g_pBitonic32PreSortCS);
        CreatePSO(pBitonicSort->s_Bitonic32InnerSortCS, g_pBitonic32InnerSortCS);
        CreatePSO(pBitonicSort->s_Bitonic32OuterSortCS, g_pBitonic32OuterSortCS);
        CreatePSO(pBitonicSort->s_Bitonic64PreSortCS, g_pBitonic64PreSortCS);
        CreatePSO(pBitonicSort->s_Bitonic64InnerSortCS, g_pBitonic64InnerSortCS);
        CreatePSO(pBitonicSort->s_Bitonic64OuterSortCS, g_pBitonic64OuterSortCS);

#undef CreatePSO
    }

    //static void DestroyCommonState(void)
    //{
    //    DispatchIndirectCommandSignature.Destroy();
    //    DrawIndirectCommandSignature.Destroy();
    //
    //    BitonicSort::Shutdown();
    //}
    //
    //static void DestroyBitonicSort(void)
    //{
    //    s_DispatchArgs.Destroy();
    //}

	static void Initialize(sloGraphicsCore* pCore, sloFrameCounter* pFrameCounter, sloGraphicsCommon* pCommon, 
        sloCommandListManager* pCmdMgr, sloBitonicSort* pBitonicSort, HWND mhWnd)
	{
        ASSERT(pCore->mSwapChain1 == nullptr, "Graphics has already been initialized");

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
                pCore->mDevice = pDevice.Detach();
        }

        if (pCore->mDevice == nullptr)
        {
            if (bUseWarpDriver)
                Utility::Print("WARP software adapter requested.  Initializing...\n");
            else
                Utility::Print("Failed to find a hardware adapter.  Falling back to WARP.\n");
            ASSERT_SUCCEEDED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter)));
            ASSERT_SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, MY_IID_PPV_ARGS(&pDevice)));
            pCore->mDevice = pDevice.Detach();
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
                pCore->mDevice->SetStablePowerState(TRUE);
        }
#endif    

#if _DEBUG
        ID3D12InfoQueue* pInfoQueue = nullptr;
        if (SUCCEEDED(pCore->mDevice->QueryInterface(MY_IID_PPV_ARGS(&pInfoQueue))))
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
        if (SUCCEEDED(pCore->mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData))))
        {
            if (FeatureData.TypedUAVLoadAdditionalFormats)
            {
                D3D12_FEATURE_DATA_FORMAT_SUPPORT Support =
                {
                    DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE
                };

                if (SUCCEEDED(pCore->mDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
                    (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
                {
                    pCore->mbTypedUAVLoadSupport_R11G11B10_FLOAT = true;
                }

                Support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

                if (SUCCEEDED(pCore->mDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
                    (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
                {
                    pCore->mbTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
                }
            }
        }

        slpCommandManagement::CreateCommandListManager(pCore, pCmdMgr);

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = pCore->mDisplayWidth;
        swapChainDesc.Height = pCore->mDisplayHeight;
        swapChainDesc.Format = SWAP_CHAIN_FORMAT;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) // Win32
        ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(pCmdMgr->GetCommandQueue(), mhWnd, &swapChainDesc, nullptr, nullptr, &pCore->mSwapChain1));
#else // UWP
        ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForCoreWindow(pCmdMgr->GetCommandQueue(), (IUnknown*)GameCore::pCore->mwindow.Get(), &swapChainDesc, nullptr, &pCore->mSwapChain1));
#endif

#if CONDITIONALLY_ENABLE_HDR_OUTPUT && defined(NTDDI_WIN10_RS2) && (NTDDI_VERSION >= NTDDI_WIN10_RS2)
        {
            IDXGISwapChain4* swapChain = (IDXGISwapChain4*)pCore->mSwapChain1;
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
                pCore->mbEnableHDROutput = true;
            }
        }
#endif

        for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        {
            ComPtr<ID3D12Resource> DisplayPlane;
            ASSERT_SUCCEEDED(pCore->mSwapChain1->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane)));
            pCore->mDisplayPlane[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
        }

        // Common state was moved to GraphicsCommon.*
        InitializeCommonState(pCommon);

        InitializeBitonicSort(pBitonicSort);

        InitializeRootSignature(pCore, pCommon);

        InitializePSOs(pCore, pCommon);

        //GpuTimeManager::Initialize(4096);
        InitializeGpuTimeManager(4096);

        SetNativeResolution();
        TemporalEffects::Initialize();
        PostEffects::Initialize();
        SSAO::Initialize();
        TextRenderer::Initialize();
        GraphRenderer::Initialize();
        ParticleEffects::Initialize(kMaxNativeWidth, kMaxNativeHeight);
	}

    static void InitializeRootSignature(sloGraphicsCore* pCore, sloGraphicsCommon* pCommon)
    {
        pCore->mPresentRS.Reset(4, 2);
        pCore->mPresentRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
        pCore->mPresentRS[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
        pCore->mPresentRS[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
        pCore->mPresentRS[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        pCore->mPresentRS.InitStaticSampler(0, pCommon->SamplerLinearClampDesc);
        pCore->mPresentRS.InitStaticSampler(1, pCommon->SamplerPointClampDesc);
        pCore->mPresentRS.Finalize(L"Present");
    }

    static void InitializePSOs(sloGraphicsCore* pCore, sloGraphicsCommon* pCommon)
    {
        // Initialize PSOs
        pCore->mBlendUIPSO.SetRootSignature(pCore->mPresentRS);
        pCore->mBlendUIPSO.SetRasterizerState(pCommon->RasterizerTwoSided);
        pCore->mBlendUIPSO.SetBlendState(pCommon->BlendPreMultiplied);
        pCore->mBlendUIPSO.SetDepthStencilState(pCommon->DepthStateDisabled);
        pCore->mBlendUIPSO.SetSampleMask(0xFFFFFFFF);
        pCore->mBlendUIPSO.SetInputLayout(0, nullptr);
        pCore->mBlendUIPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        pCore->mBlendUIPSO.SetVertexShader(g_pScreenQuadVS, sizeof(g_pScreenQuadVS));
        pCore->mBlendUIPSO.SetPixelShader(g_pBufferCopyPS, sizeof(g_pBufferCopyPS));
        pCore->mBlendUIPSO.SetRenderTargetFormat(SWAP_CHAIN_FORMAT, DXGI_FORMAT_UNKNOWN);
        pCore->mBlendUIPSO.Finalize();

#define CreatePSO( ObjName, ShaderByteCode ) \
        ObjName = pCore->mBlendUIPSO; \
        ObjName.SetBlendState( pCommon->BlendDisable ); \
        ObjName.SetPixelShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
        ObjName.Finalize();

        CreatePSO(pCore->PresentSDRPS, g_pPresentSDRPS);
        CreatePSO(pCore->MagnifyPixelsPS, g_pMagnifyPixelsPS);
        CreatePSO(pCore->BilinearUpsamplePS, g_pBilinearUpsamplePS);
        CreatePSO(pCore->BicubicHorizontalUpsamplePS, g_pBicubicHorizontalUpsamplePS);
        CreatePSO(pCore->BicubicVerticalUpsamplePS, g_pBicubicVerticalUpsamplePS);
        CreatePSO(pCore->SharpeningUpsamplePS, g_pSharpeningUpsamplePS);

#undef CreatePSO

        pCore->BicubicHorizontalUpsamplePS = pCore->mBlendUIPSO;
        pCore->BicubicHorizontalUpsamplePS.SetBlendState(pCommon->BlendDisable);
        pCore->BicubicHorizontalUpsamplePS.SetPixelShader(g_pBicubicHorizontalUpsamplePS, sizeof(g_pBicubicHorizontalUpsamplePS));
        pCore->BicubicHorizontalUpsamplePS.SetRenderTargetFormat(DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_UNKNOWN);
        pCore->BicubicHorizontalUpsamplePS.Finalize();

        pCore->PresentHDRPS = pCore->PresentSDRPS;
        pCore->PresentHDRPS.SetPixelShader(g_pPresentHDRPS, sizeof(g_pPresentHDRPS));
        DXGI_FORMAT SWAP_CHAIN_FORMATs[2] = { DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM };
        pCore->PresentHDRPS.SetRenderTargetFormats(2, SWAP_CHAIN_FORMATs, DXGI_FORMAT_UNKNOWN);
        pCore->PresentHDRPS.Finalize();

        pCore->mGenerateMipsRS.Reset(3, 1);
        pCore->mGenerateMipsRS[0].InitAsConstants(0, 4);
        pCore->mGenerateMipsRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
        pCore->mGenerateMipsRS[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 4);
        pCore->mGenerateMipsRS.InitStaticSampler(0, pCommon->SamplerLinearClampDesc);
        pCore->mGenerateMipsRS.Finalize(L"Generate Mips");

#define CreatePSO(ObjName, ShaderByteCode ) \
        ObjName.SetRootSignature(pCore->mGenerateMipsRS); \
        ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
        ObjName.Finalize();

        CreatePSO(pCore->mGenerateMipsLinearPSO[0], g_pGenerateMipsLinearCS);
        CreatePSO(pCore->mGenerateMipsLinearPSO[1], g_pGenerateMipsLinearOddXCS);
        CreatePSO(pCore->mGenerateMipsLinearPSO[2], g_pGenerateMipsLinearOddYCS);
        CreatePSO(pCore->mGenerateMipsLinearPSO[3], g_pGenerateMipsLinearOddCS);
        CreatePSO(pCore->mGenerateMipsGammaPSO[0], g_pGenerateMipsGammaCS);
        CreatePSO(pCore->mGenerateMipsGammaPSO[1], g_pGenerateMipsGammaOddXCS);
        CreatePSO(pCore->mGenerateMipsGammaPSO[2], g_pGenerateMipsGammaOddYCS);
        CreatePSO(pCore->mGenerateMipsGammaPSO[3], g_pGenerateMipsGammaOddCS);

        pCore->mPreDisplayBuffer.Create(L"PreDisplay Buffer", pCore->mDisplayWidth, pCore->mDisplayHeight, 1, SWAP_CHAIN_FORMAT);
    }

    static void InitializeGpuTimeManager(sloGraphicsCore* pCore, sloCommandListManager* pCmdMgr, sloGpuTimeManager* pGpuTimeManager,
        uint32_t MaxNumTimers)
    {
        uint64_t GpuFrequency;
        pCmdMgr->GetCommandQueue()->GetTimestampFrequency(&GpuFrequency);
        pGpuTimeManager->sm_GpuTickDelta = 1.0 / static_cast<double>(GpuFrequency);

        D3D12_HEAP_PROPERTIES HeapProps;
        HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
        HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProps.CreationNodeMask = 1;
        HeapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC BufferDesc;
        BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        BufferDesc.Alignment = 0;
        BufferDesc.Width = sizeof(uint64_t) * MaxNumTimers * 2;
        BufferDesc.Height = 1;
        BufferDesc.DepthOrArraySize = 1;
        BufferDesc.MipLevels = 1;
        BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        BufferDesc.SampleDesc.Count = 1;
        BufferDesc.SampleDesc.Quality = 0;
        BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ASSERT_SUCCEEDED(pCore->mDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, MY_IID_PPV_ARGS(&pGpuTimeManager->sm_ReadBackBuffer)));
        pGpuTimeManager->sm_ReadBackBuffer->SetName(L"GpuTimeStamp Buffer");

        D3D12_QUERY_HEAP_DESC QueryHeapDesc;
        QueryHeapDesc.Count = MaxNumTimers * 2;
        QueryHeapDesc.NodeMask = 1;
        QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        ASSERT_SUCCEEDED(pCore->mDevice->CreateQueryHeap(&QueryHeapDesc, MY_IID_PPV_ARGS(&pGpuTimeManager->sm_QueryHeap)));
        pGpuTimeManager->sm_QueryHeap->SetName(L"GpuTimeStamp QueryHeap");

        pGpuTimeManager->sm_MaxNumTimers = (uint32_t)MaxNumTimers;
    }

    static void SetNativeResolution(sloGraphicsCore* pCore, sloCommandListManager* pCmdMgr)
    {
        uint32_t NativeWidth, NativeHeight;

        switch (sloGraphicsCore::eResolution((int)pCore->TargetResolution))
        {
        default:
        case sloGraphicsCore::k720p:
            NativeWidth = 1280;
            NativeHeight = 720;
            break;
        case sloGraphicsCore::k900p:
            NativeWidth = 1600;
            NativeHeight = 900;
            break;
        case sloGraphicsCore::k1080p:
            NativeWidth = 1920;
            NativeHeight = 1080;
            break;
        case sloGraphicsCore::k1440p:
            NativeWidth = 2560;
            NativeHeight = 1440;
            break;
        case sloGraphicsCore::k1800p:
            NativeWidth = 3200;
            NativeHeight = 1800;
            break;
        case sloGraphicsCore::k2160p:
            NativeWidth = 3840;
            NativeHeight = 2160;
            break;
        }

        if (pCore->mNativeWidth == NativeWidth && pCore->mNativeHeight == NativeHeight)
            return;

        DEBUGPRINT("Changing native resolution to %ux%u", NativeWidth, NativeHeight);

        pCore->mNativeWidth = NativeWidth;
        pCore->mNativeHeight = NativeHeight;

        slpCommandManagement::IdleGPU(pCmdMgr);

        InitializeRenderingBuffers(NativeWidth, NativeHeight);
    }

    static void InitializeRenderingBuffers(sloBufferManager* pBufMgr, uint32_t bufferWidth, uint32_t bufferHeight)
    {
        GraphicsContext& InitContext = GraphicsContext::Begin();

        const uint32_t bufferWidth1 = (bufferWidth + 1) / 2;
        const uint32_t bufferWidth2 = (bufferWidth + 3) / 4;
        const uint32_t bufferWidth3 = (bufferWidth + 7) / 8;
        const uint32_t bufferWidth4 = (bufferWidth + 15) / 16;
        const uint32_t bufferWidth5 = (bufferWidth + 31) / 32;
        const uint32_t bufferWidth6 = (bufferWidth + 63) / 64;
        const uint32_t bufferHeight1 = (bufferHeight + 1) / 2;
        const uint32_t bufferHeight2 = (bufferHeight + 3) / 4;
        const uint32_t bufferHeight3 = (bufferHeight + 7) / 8;
        const uint32_t bufferHeight4 = (bufferHeight + 15) / 16;
        const uint32_t bufferHeight5 = (bufferHeight + 31) / 32;
        const uint32_t bufferHeight6 = (bufferHeight + 63) / 64;

        EsramAllocator esram;

        esram.PushStack();

        pBufMgr->m_SceneColorBuffer.Create(L"Main Color Buffer", bufferWidth, bufferHeight, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_VelocityBuffer.Create(L"Motion Vectors", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R32_UINT);
        pBufMgr->m_PostEffectsBuffer.Create(L"Post Effects Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R32_UINT);

        esram.PushStack();    // Render HDR image

        pBufMgr->m_LinearDepth[0].Create(L"Linear Depth 0", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16_UNORM);
        pBufMgr->m_LinearDepth[1].Create(L"Linear Depth 1", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16_UNORM);
        pBufMgr->m_MinMaxDepth8.Create(L"MinMaxDepth 8x8", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R32_UINT, esram);
        pBufMgr->m_MinMaxDepth16.Create(L"MinMaxDepth 16x16", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R32_UINT, esram);
        pBufMgr->m_MinMaxDepth32.Create(L"MinMaxDepth 32x32", bufferWidth5, bufferHeight5, 1, DXGI_FORMAT_R32_UINT, esram);

        pBufMgr->m_SceneDepthBuffer.Create(L"Scene Depth Buffer", bufferWidth, bufferHeight, DSV_FORMAT, esram);

        esram.PushStack(); // Begin opaque geometry

        esram.PushStack();    // Begin Shading

        pBufMgr->m_SSAOFullScreen.Create(L"SSAO Full Res", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R8_UNORM);

        esram.PushStack();    // Begin generating SSAO
        pBufMgr->m_DepthDownsize1.Create(L"Depth Down-Sized 1", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R32_FLOAT, esram);
        pBufMgr->m_DepthDownsize2.Create(L"Depth Down-Sized 2", bufferWidth2, bufferHeight2, 1, DXGI_FORMAT_R32_FLOAT, esram);
        pBufMgr->m_DepthDownsize3.Create(L"Depth Down-Sized 3", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R32_FLOAT, esram);
        pBufMgr->m_DepthDownsize4.Create(L"Depth Down-Sized 4", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R32_FLOAT, esram);
        pBufMgr->m_DepthTiled1.CreateArray(L"Depth De-Interleaved 1", bufferWidth3, bufferHeight3, 16, DXGI_FORMAT_R16_FLOAT, esram);
        pBufMgr->m_DepthTiled2.CreateArray(L"Depth De-Interleaved 2", bufferWidth4, bufferHeight4, 16, DXGI_FORMAT_R16_FLOAT, esram);
        pBufMgr->m_DepthTiled3.CreateArray(L"Depth De-Interleaved 3", bufferWidth5, bufferHeight5, 16, DXGI_FORMAT_R16_FLOAT, esram);
        pBufMgr->m_DepthTiled4.CreateArray(L"Depth De-Interleaved 4", bufferWidth6, bufferHeight6, 16, DXGI_FORMAT_R16_FLOAT, esram);
        pBufMgr->m_AOMerged1.Create(L"AO Re-Interleaved 1", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOMerged2.Create(L"AO Re-Interleaved 2", bufferWidth2, bufferHeight2, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOMerged3.Create(L"AO Re-Interleaved 3", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOMerged4.Create(L"AO Re-Interleaved 4", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOSmooth1.Create(L"AO Smoothed 1", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOSmooth2.Create(L"AO Smoothed 2", bufferWidth2, bufferHeight2, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOSmooth3.Create(L"AO Smoothed 3", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOHighQuality1.Create(L"AO High Quality 1", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOHighQuality2.Create(L"AO High Quality 2", bufferWidth2, bufferHeight2, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOHighQuality3.Create(L"AO High Quality 3", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_AOHighQuality4.Create(L"AO High Quality 4", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R8_UNORM, esram);
        esram.PopStack();    // End generating SSAO

        pBufMgr->m_ShadowBuffer.Create(L"Shadow Map", 2048, 2048, esram);

        esram.PopStack();    // End Shading

        esram.PushStack();    // Begin depth of field
        pBufMgr->m_DoFTileClass[0].Create(L"DoF Tile Classification Buffer 0", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R11G11B10_FLOAT);
        pBufMgr->m_DoFTileClass[1].Create(L"DoF Tile Classification Buffer 1", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R11G11B10_FLOAT);

        pBufMgr->m_DoFPresortBuffer.Create(L"DoF Presort Buffer", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R11G11B10_FLOAT, esram);
        pBufMgr->m_DoFPrefilter.Create(L"DoF PreFilter Buffer", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R11G11B10_FLOAT, esram);
        pBufMgr->m_DoFBlurColor[0].Create(L"DoF Blur Color", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R11G11B10_FLOAT, esram);
        pBufMgr->m_DoFBlurColor[1].Create(L"DoF Blur Color", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R11G11B10_FLOAT, esram);
        pBufMgr->m_DoFBlurAlpha[0].Create(L"DoF FG Alpha", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_DoFBlurAlpha[1].Create(L"DoF FG Alpha", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_DoFWorkQueue.Create(L"DoF Work Queue", bufferWidth4 * bufferHeight4, 4, esram);
        pBufMgr->m_DoFFastQueue.Create(L"DoF Fast Queue", bufferWidth4 * bufferHeight4, 4, esram);
        pBufMgr->m_DoFFixupQueue.Create(L"DoF Fixup Queue", bufferWidth4 * bufferHeight4, 4, esram);
        esram.PopStack();    // End depth of field

        pBufMgr->m_TemporalColor[0].Create(L"Temporal Color 0", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
        pBufMgr->m_TemporalColor[1].Create(L"Temporal Color 1", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
        TemporalEffects::ClearHistory(InitContext);

        esram.PushStack();    // Begin motion blur
        pBufMgr->m_MotionPrepBuffer.Create(L"Motion Blur Prep", bufferWidth1, bufferHeight1, 1, HDR_MOTION_FORMAT, esram);
        esram.PopStack();    // End motion blur

        esram.PopStack();    // End opaque geometry

        esram.PopStack();    // End HDR image

        esram.PushStack();    // Begin post processing

            // This is useful for storing per-pixel weights such as motion strength or pixel luminance
        pBufMgr->m_LumaBuffer.Create(L"Luminance", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R8_UNORM, esram);
        pBufMgr->m_Histogram.Create(L"Histogram", 256, 4, esram);

        // Divisible by 128 so that after dividing by 16, we still have multiples of 8x8 tiles.  The bloom
        // dimensions must be at least 1/4 native resolution to avoid undersampling.
        //uint32_t kBloomWidth = bufferWidth > 2560 ? Math::AlignUp(bufferWidth / 4, 128) : 640;
        //uint32_t kBloomHeight = bufferHeight > 1440 ? Math::AlignUp(bufferHeight / 4, 128) : 384;
        uint32_t kBloomWidth = bufferWidth > 2560 ? 1280 : 640;
        uint32_t kBloomHeight = bufferHeight > 1440 ? 768 : 384;

        esram.PushStack();    // Begin bloom and tone mapping
        pBufMgr->m_LumaLR.Create(L"Luma Buffer", kBloomWidth, kBloomHeight, 1, DXGI_FORMAT_R8_UINT, esram);
        pBufMgr->m_aBloomUAV1[0].Create(L"Bloom Buffer 1a", kBloomWidth, kBloomHeight, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_aBloomUAV1[1].Create(L"Bloom Buffer 1b", kBloomWidth, kBloomHeight, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_aBloomUAV2[0].Create(L"Bloom Buffer 2a", kBloomWidth / 2, kBloomHeight / 2, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_aBloomUAV2[1].Create(L"Bloom Buffer 2b", kBloomWidth / 2, kBloomHeight / 2, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_aBloomUAV3[0].Create(L"Bloom Buffer 3a", kBloomWidth / 4, kBloomHeight / 4, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_aBloomUAV3[1].Create(L"Bloom Buffer 3b", kBloomWidth / 4, kBloomHeight / 4, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_aBloomUAV4[0].Create(L"Bloom Buffer 4a", kBloomWidth / 8, kBloomHeight / 8, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_aBloomUAV4[1].Create(L"Bloom Buffer 4b", kBloomWidth / 8, kBloomHeight / 8, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_aBloomUAV5[0].Create(L"Bloom Buffer 5a", kBloomWidth / 16, kBloomHeight / 16, 1, DefaultHdrColorFormat, esram);
        pBufMgr->m_aBloomUAV5[1].Create(L"Bloom Buffer 5b", kBloomWidth / 16, kBloomHeight / 16, 1, DefaultHdrColorFormat, esram);
        esram.PopStack();    // End tone mapping

        esram.PushStack();    // Begin antialiasing
        const uint32_t kFXAAWorkSize = bufferWidth * bufferHeight / 4 + 128;
        pBufMgr->m_FXAAWorkQueue.Create(L"FXAA Work Queue", kFXAAWorkSize, sizeof(uint32_t), esram);
        pBufMgr->m_FXAAColorQueue.Create(L"FXAA Color Queue", kFXAAWorkSize, sizeof(uint32_t), esram);
        pBufMgr->m_FXAAWorkCounters.Create(L"FXAA Work Counters", 2, sizeof(uint32_t));
        InitContext.ClearUAV(pBufMgr->m_FXAAWorkCounters);
        esram.PopStack();    // End antialiasing

        esram.PopStack();    // End post processing

        esram.PushStack(); // GenerateMipMaps() test
        pBufMgr->m_GenMipsBuffer.Create(L"GenMips", bufferWidth, bufferHeight, 0, DXGI_FORMAT_R11G11B10_FLOAT, esram);
        esram.PopStack();

        pBufMgr->m_OverlayBuffer.Create(L"UI Overlay", pBufMgr->m_DisplayWidth, pBufMgr->m_DisplayHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM, esram);
        pBufMgr->m_HorizontalBuffer.Create(L"Bicubic Intermediate", pBufMgr->m_DisplayWidth, bufferHeight, 1, DefaultHdrColorFormat, esram);

        esram.PopStack(); // End final image

        InitContext.Finish();
    }

};

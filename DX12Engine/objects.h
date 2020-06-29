#pragma once
#include "pch.h"
#include "SamplerDesc.h"
#include "ColorBuffer.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "CommandSignature.h"
#include "GpuBuffer.h"
#include "DescriptorHeap.h"
#include "DepthBuffer.h"
#include "ShadowBuffer.h"
#include "EsramAllocator.h"

using namespace Microsoft::WRL;

constexpr int SWAP_CHAIN_BUFFER_COUNT = 3;
constexpr DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_R10G10B10A2_UNORM;

struct sloGraphicsCore
{
    enum eResolution { k720p, k900p, k1080p, k1440p, k1800p, k2160p };
    
#ifndef RELEASE
    const GUID WKPDID_D3DDebugObjectName = { 0x429b8c22,0x9188,0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
#endif

    const uint32_t kMaxNativeWidth = 3840;
    const uint32_t kMaxNativeHeight = 2160;
    const uint32_t kNumPredefinedResolutions = 6;

    const char* ResolutionLabels[6] = { "1280x720", "1600x900", "1920x1080", "2560x1440", "3200x1800", "3840x2160" };
    EnumVar TargetResolution{ "Graphics/Display/Native Resolution", k1080p, kNumPredefinedResolutions, ResolutionLabels };

    BoolVar mEnableVSync{ "Timing/VSync", true };

    bool mbTypedUAVLoadSupport_R11G11B10_FLOAT = false;
    bool mbTypedUAVLoadSupport_R16G16B16A16_FLOAT = false;
    bool mbEnableHDROutput = false;
    NumVar mHDRPaperWhite{ "Graphics/Display/Paper White (nits)", 200.0f, 100.0f, 500.0f, 50.0f };
    NumVar mMaxDisplayLuminance{ "Graphics/Display/Peak Brightness (nits)", 1000.0f, 500.0f, 10000.0f, 100.0f };
    const char* HDRModeLabels[3] = { "HDR", "SDR", "Side-by-Side" };
    EnumVar HDRDebugMode{ "Graphics/Display/HDR Debug Mode", 0, 3, HDRModeLabels };

    uint32_t mNativeWidth = 0;
    uint32_t mNativeHeight = 0;
    uint32_t mDisplayWidth = 1920;
    uint32_t mDisplayHeight = 1080;
    ColorBuffer mPreDisplayBuffer;

    ID3D12Device* mDevice;
    //CommandListManager mCommandManager;
    //ContextManager mContextManager;

    D3D_FEATURE_LEVEL mD3DFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    ColorBuffer mDisplayPlane[SWAP_CHAIN_BUFFER_COUNT];
    UINT mCurrentBuffer = 0;

    IDXGISwapChain1* mSwapChain1 = nullptr;

    DescriptorAllocator mDescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
    {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
    };

    RootSignature mPresentRS;
    GraphicsPSO mBlendUIPSO;
    GraphicsPSO PresentSDRPS;
    GraphicsPSO PresentHDRPS;
    GraphicsPSO MagnifyPixelsPS;
    GraphicsPSO SharpeningUpsamplePS;
    GraphicsPSO BicubicHorizontalUpsamplePS;
    GraphicsPSO BicubicVerticalUpsamplePS;
    GraphicsPSO BilinearUpsamplePS;

    RootSignature mGenerateMipsRS;
    ComputePSO mGenerateMipsLinearPSO[4];
    ComputePSO mGenerateMipsGammaPSO[4];

    enum { kBilinear, kBicubic, kSharpening, kFilterCount };
    const char* FilterLabels[3] = { "Bilinear", "Bicubic", "Sharpening" };
    EnumVar UpsampleFilter{ "Graphics/Display/Upsample Filter", kFilterCount - 1, kFilterCount, FilterLabels };
    NumVar BicubicUpsampleWeight{ "Graphics/Display/Bicubic Filter Weight", -0.75f, -1.0f, -0.25f, 0.25f };
    NumVar SharpeningSpread{ "Graphics/Display/Sharpness Sample Spread", 1.0f, 0.7f, 2.0f, 0.1f };
    NumVar SharpeningRotation{"Graphics/Display/Sharpness Sample Rotation", 45.0f, 0.0f, 90.0f, 15.0f};
    NumVar SharpeningStrength{ "Graphics/Display/Sharpness Strength", 0.10f, 0.0f, 1.0f, 0.01f };

    enum DebugZoomLevel { kDebugZoomOff, kDebugZoom2x, kDebugZoom4x, kDebugZoom8x, kDebugZoom16x, kDebugZoomCount };
    const char* DebugZoomLabels[5] = { "Off", "2x Zoom", "4x Zoom", "8x Zoom", "16x Zoom" };
    EnumVar DebugZoom{ "Graphics/Display/Magnify Pixels", kDebugZoomOff, kDebugZoomCount, DebugZoomLabels };

    inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
    {
        return mDescriptorAllocator[Type].Allocate(Count);
    }
    
};

struct sloFrameCounter
{
    float mFrameTime = 0.0f;
    uint64_t mFrameIndex = 0;
    int64_t mFrameStartTick = 0;

    BoolVar mLimitTo30Hz{ "Timing/Limit To 30Hz", false };
    BoolVar mDropRandomFrames{ "Timing/Drop Random Frames", false };

    // Returns the number of elapsed frames since application start
    inline uint64_t GetFrameCount(void)
    {
        return mFrameIndex;
    }

    // The amount of time elapsed during the last completed frame.  The CPU and/or
    // GPU may be idle during parts of the frame.  The frame time measures the time
    // between calls to present each frame.
    inline float GetFrameTime(void)
    {
        return mFrameTime;
    }

    // The total number of frames per second
    inline float GetFrameRate(void)
    {
        return mFrameTime == 0.0f ? 0.0f : 1.0f / mFrameTime;
    }

};

struct sloGraphicsCommon
{
    SamplerDesc SamplerLinearWrapDesc;
    SamplerDesc SamplerAnisoWrapDesc;
    SamplerDesc SamplerShadowDesc;
    SamplerDesc SamplerLinearClampDesc;
    SamplerDesc SamplerVolumeWrapDesc;
    SamplerDesc SamplerPointClampDesc;
    SamplerDesc SamplerPointBorderDesc;
    SamplerDesc SamplerLinearBorderDesc;

    D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerAnisoWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerShadow;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearClamp;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerVolumeWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointClamp;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointBorder;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearBorder;

    D3D12_RASTERIZER_DESC RasterizerDefault;    // Counter-clockwise
    D3D12_RASTERIZER_DESC RasterizerDefaultMsaa;
    D3D12_RASTERIZER_DESC RasterizerDefaultCw;    // Clockwise winding
    D3D12_RASTERIZER_DESC RasterizerDefaultCwMsaa;
    D3D12_RASTERIZER_DESC RasterizerTwoSided;
    D3D12_RASTERIZER_DESC RasterizerTwoSidedMsaa;
    D3D12_RASTERIZER_DESC RasterizerShadow;
    D3D12_RASTERIZER_DESC RasterizerShadowCW;
    D3D12_RASTERIZER_DESC RasterizerShadowTwoSided;

    D3D12_BLEND_DESC BlendNoColorWrite;
    D3D12_BLEND_DESC BlendDisable;
    D3D12_BLEND_DESC BlendPreMultiplied;
    D3D12_BLEND_DESC BlendTraditional;
    D3D12_BLEND_DESC BlendAdditive;
    D3D12_BLEND_DESC BlendTraditionalAdditive;

    D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
    D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite;
    D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly;
    D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
    D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual;

    CommandSignature DispatchIndirectCommandSignature{1};
    CommandSignature DrawIndirectCommandSignature{1};

};

struct sloBitonicSort
{
    IndirectArgsBuffer s_DispatchArgs;

    RootSignature s_RootSignature;
    ComputePSO s_BitonicIndirectArgsCS;
    ComputePSO s_Bitonic32PreSortCS;
    ComputePSO s_Bitonic32InnerSortCS;
    ComputePSO s_Bitonic32OuterSortCS;
    ComputePSO s_Bitonic64PreSortCS;
    ComputePSO s_Bitonic64InnerSortCS;
    ComputePSO s_Bitonic64OuterSortCS;

};

struct sloCommandAllocatorPool
{
    const D3D12_COMMAND_LIST_TYPE m_cCommandListType;

    std::vector<ID3D12CommandAllocator*> m_AllocatorPool;
    std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_ReadyAllocators;
    std::mutex m_AllocatorMutex;

    inline size_t Size() { return m_AllocatorPool.size(); }

};

struct sloCommandQueue
{
    ID3D12CommandQueue* m_CommandQueue;

    const D3D12_COMMAND_LIST_TYPE m_Type;

    sloCommandAllocatorPool m_AllocatorPool;
    std::mutex m_FenceMutex;
    std::mutex m_EventMutex;

    // Lifetime of these objects is managed by the descriptor cache
    ID3D12Fence* m_pFence;
    uint64_t m_NextFenceValue;
    uint64_t m_LastCompletedFenceValue;
    HANDLE m_FenceEventHandle;

    inline bool IsReady()
    {
        return m_CommandQueue != nullptr;
    }

    ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue; }

    uint64_t GetNextFenceValue() { return m_NextFenceValue; }

    uint64_t IncrementFence(void)
    {
        std::lock_guard<std::mutex> LockGuard(m_FenceMutex);
        m_CommandQueue->Signal(m_pFence, m_NextFenceValue);
        return m_NextFenceValue++;
    }

    bool IsFenceComplete(uint64_t FenceValue)
    {
        // Avoid querying the fence value by testing against the last one seen.
        // The max() is to protect against an unlikely race condition that could cause the last
        // completed fence value to regress.
        if (FenceValue > m_LastCompletedFenceValue)
            m_LastCompletedFenceValue = std::max<uint64_t>(m_LastCompletedFenceValue, m_pFence->GetCompletedValue());

        return FenceValue <= m_LastCompletedFenceValue;
    }

    void WaitForFence(uint64_t FenceValue)
    {
        if (IsFenceComplete(FenceValue))
            return;

        // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
        // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
        // the fence can only have one event set on completion, then thread B has to wait for 
        // 100 before it knows 99 is ready.  Maybe insert sequential events?
        {
            std::lock_guard<std::mutex> LockGuard(m_EventMutex);

            m_pFence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
            WaitForSingleObject(m_FenceEventHandle, INFINITE);
            m_LastCompletedFenceValue = FenceValue;
        }
    }

    void WaitForIdle(void) { WaitForFence(IncrementFence()); }

};

struct sloCommandListManager
{
    sloCommandQueue m_GraphicsQueue;
    sloCommandQueue m_ComputeQueue;
    sloCommandQueue m_CopyQueue;

    sloCommandQueue& GetGraphicsQueue(void) { return m_GraphicsQueue; }
    sloCommandQueue& GetComputeQueue(void) { return m_ComputeQueue; }
    sloCommandQueue& GetCopyQueue(void) { return m_CopyQueue; }

    sloCommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        switch (Type)
        {
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_ComputeQueue;
        case D3D12_COMMAND_LIST_TYPE_COPY: return m_CopyQueue;
        default: return m_GraphicsQueue;
        }
    }

    ID3D12CommandQueue* GetCommandQueue()
    {
        return m_GraphicsQueue.GetCommandQueue();
    }

};

struct sloGpuTimeManager
{
    ID3D12QueryHeap* sm_QueryHeap = nullptr;
    ID3D12Resource* sm_ReadBackBuffer = nullptr;
    uint64_t* sm_TimeStampBuffer = nullptr;
    uint64_t sm_Fence = 0;
    uint32_t sm_MaxNumTimers = 0;
    uint32_t sm_NumTimers = 1;
    uint64_t sm_ValidTimeStart = 0;
    uint64_t sm_ValidTimeEnd = 0;
    double sm_GpuTickDelta = 0.0;
};

struct sloBufferManager
{
    DepthBuffer m_SceneDepthBuffer;
    ColorBuffer m_SceneColorBuffer;
    ColorBuffer m_PostEffectsBuffer;
    ColorBuffer m_VelocityBuffer;
    ColorBuffer m_OverlayBuffer;
    ColorBuffer m_HorizontalBuffer;

    ShadowBuffer m_ShadowBuffer;

    ColorBuffer m_SSAOFullScreen{ Color(1.0f, 1.0f, 1.0f) };
    ColorBuffer m_LinearDepth[2];
    ColorBuffer m_MinMaxDepth8;
    ColorBuffer m_MinMaxDepth16;
    ColorBuffer m_MinMaxDepth32;
    ColorBuffer m_DepthDownsize1;
    ColorBuffer m_DepthDownsize2;
    ColorBuffer m_DepthDownsize3;
    ColorBuffer m_DepthDownsize4;
    ColorBuffer m_DepthTiled1;
    ColorBuffer m_DepthTiled2;
    ColorBuffer m_DepthTiled3;
    ColorBuffer m_DepthTiled4;
    ColorBuffer m_AOMerged1;
    ColorBuffer m_AOMerged2;
    ColorBuffer m_AOMerged3;
    ColorBuffer m_AOMerged4;
    ColorBuffer m_AOSmooth1;
    ColorBuffer m_AOSmooth2;
    ColorBuffer m_AOSmooth3;
    ColorBuffer m_AOHighQuality1;
    ColorBuffer m_AOHighQuality2;
    ColorBuffer m_AOHighQuality3;
    ColorBuffer m_AOHighQuality4;

    ColorBuffer m_DoFTileClass[2];
    ColorBuffer m_DoFPresortBuffer;
    ColorBuffer m_DoFPrefilter;
    ColorBuffer m_DoFBlurColor[2];
    ColorBuffer m_DoFBlurAlpha[2];
    StructuredBuffer m_DoFWorkQueue;
    StructuredBuffer m_DoFFastQueue;
    StructuredBuffer m_DoFFixupQueue;

    ColorBuffer m_MotionPrepBuffer;
    ColorBuffer m_LumaBuffer;
    ColorBuffer m_TemporalColor[2];
    ColorBuffer m_aBloomUAV1[2];    // 640x384 (1/3)
    ColorBuffer m_aBloomUAV2[2];    // 320x192 (1/6)  
    ColorBuffer m_aBloomUAV3[2];    // 160x96  (1/12)
    ColorBuffer m_aBloomUAV4[2];    // 80x48   (1/24)
    ColorBuffer m_aBloomUAV5[2];    // 40x24   (1/48)
    ColorBuffer m_LumaLR;
    ByteAddressBuffer m_Histogram;
    ByteAddressBuffer m_FXAAWorkCounters;
    ByteAddressBuffer m_FXAAWorkQueue;
    TypedBuffer m_FXAAColorQueue{ DXGI_FORMAT_R11G11B10_FLOAT };

    // For testing GenerateMipMaps()
    ColorBuffer m_GenMipsBuffer;

    DXGI_FORMAT DefaultHdrColorFormat = DXGI_FORMAT_R11G11B10_FLOAT;
};

// global variable
static sloGraphicsCore* gCore;
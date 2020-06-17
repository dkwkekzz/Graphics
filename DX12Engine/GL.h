#pragma once
#include "d3dx12.h"
#include "TypeDefine.h"
#include "GlobalVar.h"
#include "DescriptorHeap.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class GL
{
public:
	inline TDevice* GetDevice()const { return md3dDevice.Get(); }
	inline TResource* CurrentBackBuffer()const { return mSwapChainBuffer[mCurrBackBuffer].Get(); }
	inline D3D_DRIVER_TYPE GetDeviceType()const { return md3dDriverType; }
	inline DXGI_FORMAT GetBackBufferFormat()const { return mBackBufferFormat; }
	inline DXGI_FORMAT GetDepthStencilFormat()const { return mDepthStencilFormat; }
	inline bool Get4xMsaaState()const { return m4xMsaaState; }
	inline UINT Get4xMsaaQuality()const { return m4xMsaaQuality; }

	inline ID3D12GraphicsCommandList* GetCommandList()const { return mCommandList.Get(); }
	inline D3D12_VIEWPORT GetViewport()const { return mScreenViewport; }

	void Init(HWND mainWnd);
	void Resize(const DescriptorHeap&, int, int);
	void FlushCommandQueue();

	bool Get4xMsaaState()const;
	void Set4xMsaaState(bool value);

private:
	void CreateCommandObjects();
	void CreateSwapChain();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

private:
	HWND mMainWnd;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	int mCurrBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[Global::SWAP_CHAIN_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Set true to use 4X MSAA (?.1.8).  The default is false.
	bool		m4xMsaaState = false;    // 4X MSAA enabled
	UINT		m4xMsaaQuality = 0;      // quality level of 4X MSAA

	int				mClientWidth = Global::SCREEN_WIDTH;
	int				mClientHeight = Global::SCREEN_HEIGHT;

};


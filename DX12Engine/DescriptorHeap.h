#pragma once
#include "d3dx12.h"

class DescriptorHeap
{
public:
    inline D3D12_CPU_DESCRIPTOR_HANDLE BackBufferView(int mCurrBackBuffer)const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize); }
    inline D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const { return mDsvHeap->GetCPUDescriptorHandleForHeapStart(); }
    inline CD3DX12_CPU_DESCRIPTOR_HANDLE SrvHeapHandle()const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvHeap->GetCPUDescriptorHandleForHeapStart()); }
    inline CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHeapHandle()const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart()); }
    inline CD3DX12_CPU_DESCRIPTOR_HANDLE DsvHeapHandle()const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart()); }
    inline int SrvDescriptorSize()const { return mCbvSrvUavDescriptorSize; }
    inline int RtvDescriptorSize()const { return mRtvDescriptorSize; }
    inline int DsvDescriptorSize()const { return mDsvDescriptorSize; }

    void Init(ID3D12Device* d3dDevice);

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap = nullptr;

    UINT mCbvSrvUavDescriptorSize = 0;
    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;

};


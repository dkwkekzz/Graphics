#pragma once

#include "d3dUtil.h"

class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT capacity, bool isConstantBuffer);
    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer();

    ID3D12Resource* Resource() const;

    void* PushData(void* src, int length);
    void* CopyData(void* src, void* dest, int length);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    bool mIsConstantBuffer = false;
    UINT mOffset = 0;
};
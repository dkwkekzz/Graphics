#include "pch.h"
#include "UploadBuffer.h"

UploadBuffer::UploadBuffer(ID3D12Device* device, UINT capacity, bool isConstantBuffer) :
    mIsConstantBuffer(isConstantBuffer)
{
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(capacity),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mUploadBuffer)));

    ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

    // We do not need to unmap until we are done with the resource.  However, we must not write to
    // the resource while it is in use by the GPU (so we must use synchronization techniques).
}

UploadBuffer::~UploadBuffer()
{
    if (mUploadBuffer != nullptr)
        mUploadBuffer->Unmap(0, nullptr);

    mMappedData = nullptr;
}

ID3D12Resource* UploadBuffer::Resource() const
{
    return mUploadBuffer.Get();
}

void* UploadBuffer::PushData(void* src, int length)
{
    void* dest = &mMappedData[mOffset];
    memcpy(dest, src, length);

    // Constant buffer elements need to be multiples of 256 bytes.
    // This is because the hardware can only view constant data 
    // at m*256 byte offsets and of n*256 byte lengths. 
    // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
    // UINT64 OffsetInBytes; // multiple of 256
    // UINT   SizeInBytes;   // multiple of 256
    // } D3D12_CONSTANT_BUFFER_VIEW_DESC;
    UINT elementByteSize = mIsConstantBuffer ? d3dUtil::CalcConstantBufferByteSize(length) : length;
    mOffset += elementByteSize;

    return dest;
}

void* UploadBuffer::CopyData(void* src, void* dest, int length)
{
    return memcpy(dest, src, length);
}

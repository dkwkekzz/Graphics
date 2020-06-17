#pragma once
#include "TypeDefine.h"

struct FrameResource;
struct RenderBundle;

class CommandObject
{
public:
	CommandObject();
	//void Reset(ID3D12CommandAllocator* allocator, TPSO* initialState);
	void Begin(ID3D12CommandAllocator* allocator, TPSO* initialState);
	void Render(const FrameResource* currentFrameRes, const RenderBundle* bundle);
	void End(FrameResource* currentFrameRes);
	void ResourceBarrier(TResource* res);
	void Execute();
	UINT64 Fence();
	void WaitForEvent();
	void Flush();

private:
	const UINT m_passCBByteSize;
	const UINT m_objCBByteSize;
	const UINT m_matCBByteSize;

	ID3D12CommandQueue* mCommandQueue;
	//Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	ID3D12GraphicsCommandList* mCommandList;

	TPSO* m_lastPipelineState = nullptr;

	//Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	//UINT64 mCurrentFence = 0L;

};


#pragma once
#include "TypeDefine.h"

class GL;
struct FrameResource;
struct RenderBundle;

struct DrawContext
{
	FrameResource* frameRes;
	TPSO* pipelineState;
	int passCBIndex;
	int ritemCount;
	RenderItem* ritems;
	bool useStencil;
	int stencilRef;
};

class CommandObject
{
public:
	void Init(GL* gl);
	void Reset(TPSO* initialState);
	void Begin();
	void Render(const FrameResource* currentFrameRes, const RenderBundle* bundle);
	void End();

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	UINT m_passCBByteSize;
	UINT m_objCBByteSize;
	UINT m_matCBByteSize;
	TPSO* m_lastPipelineState = nullptr;
};


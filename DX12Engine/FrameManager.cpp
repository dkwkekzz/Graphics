
#include "pch.h"
#include "FrameManager.h"
#include "GlobalVar.h"

FrameResource::FrameResource(GL* gl, UINT passCount, UINT objectCount, UINT materialCount)
{
	ThrowIfFailed(gl->GetDevice()->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(gl, passCount, true);
    MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(gl, materialCount, true);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(gl, objectCount, true);
}

FrameResource::~FrameResource()
{
}

void FrameManager::Init(GL* gl, int numFrameResources, int maxObjCount, int maxMatCount)
{
    for (int i = 0; i < numFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(gl, Global::MAIN_PASS_COUNT, (UINT)maxObjCount, (UINT)maxMatCount));
    }
}

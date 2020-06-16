
#include "pch.h"
#include "FrameResource.h"
#include "GL.h"

FrameResource::FrameResource(GL* gl, UINT passCount, UINT objectCount, UINT materialCount)
{
	//ThrowIfFailed(gl->GetDevice()->CreateCommandAllocator(
	//	D3D12_COMMAND_LIST_TYPE_DIRECT,
	//	IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(gl, passCount, true);
    MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(gl, materialCount, true);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(gl, objectCount, true);
}

FrameResource::~FrameResource()
{

}
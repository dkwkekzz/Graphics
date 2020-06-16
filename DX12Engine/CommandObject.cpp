#include "pch.h"
#include "CommandObject.h"
#include "GlobalVar.h"
#include "GL.h"
#include "FrameResource.h"
#include "RenderLayers.h"
#include "RenderItem.h"

void CommandObject::Init(GL * gl)
{
	auto* device = gl->GetDevice();

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mCmdListAlloc.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCommandList->Close();
}

void CommandObject::Reset(TPSO* initialState)
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mCmdListAlloc.Get(), initialState));

	m_lastPipelineState = initialState;
}

void CommandObject::Begin()
{
	m_passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	m_objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	m_matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void CommandObject::Render(const FrameResource* currentFrameRes, const RenderBundle* bundle)
{
	auto passCB = currentFrameRes->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + bundle->passCBIndex * m_passCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(Global::PASSCB_PARAMETER_INDEX, passCBAddress);

	auto* bundleState = bundle->pipelineState;
	if (m_lastPipelineState != bundleState)
	{
		mCommandList->SetPipelineState(bundleState);
		m_lastPipelineState = bundleState;
	}

	if (bundle->useStencil)
	{
		mCommandList->OMSetStencilRef(bundle->stencilRef);
	}

	auto objectCB = currentFrameRes->ObjectCB->Resource();
	auto matCB = currentFrameRes->MaterialCB->Resource();

	// For each render item...
	const auto& ritems = bundle->ritems;
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		mCommandList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * m_objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * m_matCBByteSize;

		mCommandList->SetGraphicsRootDescriptorTable(0, tex);
		mCommandList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		mCommandList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		mCommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void CommandObject::End()
{
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % Global::SWAP_CHAIN_BUFFER_COUNT;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Notify the fence when the GPU completes commands up to this fence point.
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}


#include "pch.h"
#include "CommandObject.h"
#include "GlobalVar.h"
#include "GL.h"
#include "FrameManager.h"
#include "RenderLayers.h"
#include "RenderItem.h"

static const UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
static const UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
static const UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

CommandObject::CommandObject()
	: m_passCBByteSize(d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants)))
	, m_objCBByteSize(d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants)))
	, m_matCBByteSize(d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants)))
{
}

void CommandObject::Begin(ID3D12CommandAllocator* allocator, TPSO* initialState)
{
	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(allocator, initialState));

	m_lastPipelineState = initialState;
}

void CommandObject::Begin()
{

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
	mCommandList->SetGraphicsRootSignature(bundle->pipelineState->.Get());

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

void CommandObject::End(FrameResource* currentFrameRes)
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

}

void CommandObject::ResourceBarrier(TResource* res)
{
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void CommandObject::Execute()
{
	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

UINT64 CommandObject::Fence()
{
	// Advance the fence value to mark commands up to this fence point.
	mCurrentFence++;

	// Notify the fence when the GPU completes commands up to this fence point.
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

	return mCurrentFence;
}

void CommandObject::WaitForEvent()
{
	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void CommandObject::Flush()
{
	Fence();
	WaitForEvent();
}

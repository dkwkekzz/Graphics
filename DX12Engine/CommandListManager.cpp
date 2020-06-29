#include "pch.h"
#include "CommandListManager.h"

CommandListManager::~CommandListManager()
{
    Shutdown();
}

void CommandListManager::Shutdown()
{
    m_GraphicsQueue.Shutdown();
    m_ComputeQueue.Shutdown();
    m_CopyQueue.Shutdown();
}

void CommandListManager::Create(ID3D12Device* pDevice)
{
    ASSERT(pDevice != nullptr);

    m_Device = pDevice;

    m_GraphicsQueue.Create(pDevice, this);
    m_ComputeQueue.Create(pDevice, this);
    m_CopyQueue.Create(pDevice, this);
}

void CommandListManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList** List, ID3D12CommandAllocator** Allocator)
{
    ASSERT(Type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
    switch (Type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT: *Allocator = m_GraphicsQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE: *Allocator = m_ComputeQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_COPY: *Allocator = m_CopyQueue.RequestAllocator(); break;
    }

    ASSERT_SUCCEEDED(m_Device->CreateCommandList(1, Type, *Allocator, nullptr, MY_IID_PPV_ARGS(List)));
    (*List)->SetName(L"CommandList");
}

void CommandListManager::WaitForFence(uint64_t FenceValue)
{
    //CommandQueue& Producer = Graphics::g_CommandManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    CommandQueue& Producer = GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    Producer.WaitForFence(FenceValue);
}

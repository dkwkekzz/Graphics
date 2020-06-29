#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>
#include "CommandAllocatorPool.h"

class CommandListManager;

class CommandQueue
{
    friend class CommandListManager;
    //friend class CommandContext;

public:
    CommandQueue(D3D12_COMMAND_LIST_TYPE Type);
    ~CommandQueue();

    void Create(ID3D12Device* pDevice, CommandListManager* pCmdMgr);
    void Shutdown();

    inline bool IsReady()
    {
        return m_CommandQueue != nullptr;
    }

    uint64_t IncrementFence(void);
    bool IsFenceComplete(uint64_t FenceValue);
    void StallForFence(uint64_t FenceValue);
    void StallForProducer(CommandQueue& Producer);
    void WaitForFence(uint64_t FenceValue);
    void WaitForIdle(void) { WaitForFence(IncrementFence()); }

    ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue; }

    uint64_t GetNextFenceValue() { return m_NextFenceValue; }

private:

    uint64_t ExecuteCommandList(ID3D12CommandList* List);
    ID3D12CommandAllocator* RequestAllocator(void);
    void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);

    CommandListManager* m_CommandManager;
    ID3D12CommandQueue* m_CommandQueue;

    const D3D12_COMMAND_LIST_TYPE m_Type;

    CommandAllocatorPool m_AllocatorPool;
    std::mutex m_FenceMutex;
    std::mutex m_EventMutex;

    // Lifetime of these objects is managed by the descriptor cache
    ID3D12Fence* m_pFence;
    uint64_t m_NextFenceValue;
    uint64_t m_LastCompletedFenceValue;
    HANDLE m_FenceEventHandle;

};

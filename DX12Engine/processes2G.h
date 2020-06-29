#pragma once
#include "pch.h"
#include "objects.h"

struct slpCommandManagement
{
    static void IdleGPU(sloCommandListManager* pCmdMgr)
    {
        pCmdMgr->m_GraphicsQueue.WaitForIdle();
        pCmdMgr->m_ComputeQueue.WaitForIdle();
        pCmdMgr->m_CopyQueue.WaitForIdle();
    }

    static void CreateQueue(sloGraphicsCore* pCore, sloCommandQueue* pCmdQueue)
    {
        ASSERT(pCore->mDevice != nullptr);
        //ASSERT(pCmdMgr != nullptr);
        ASSERT(!pCmdQueue->IsReady());
        ASSERT(pCmdQueue->m_AllocatorPool.Size() == 0);

        D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
        QueueDesc.Type = pCmdQueue->m_Type;
        QueueDesc.NodeMask = 1;
        pCore->mDevice->CreateCommandQueue(&QueueDesc, MY_IID_PPV_ARGS(&pCmdQueue->m_CommandQueue));
        pCmdQueue->m_CommandQueue->SetName(L"CommandListManager::m_CommandQueue");

        ASSERT_SUCCEEDED(pCore->mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, MY_IID_PPV_ARGS(&pCmdQueue->m_pFence)));
        pCmdQueue->m_pFence->SetName(L"CommandListManager::m_pFence");
        pCmdQueue->m_pFence->Signal((uint64_t)pCmdQueue->m_Type << 56);

        pCmdQueue->m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
        ASSERT(pCmdQueue->m_FenceEventHandle != NULL);

        ASSERT(pCmdQueue->IsReady());
    }

    static void CreateCommandListManager(sloGraphicsCore* pCore, sloCommandListManager* pCmdMgr)
    {
        CreateQueue(pCore, &pCmdMgr->m_GraphicsQueue);
        CreateQueue(pCore, &pCmdMgr->m_ComputeQueue);
        CreateQueue(pCore, &pCmdMgr->m_CopyQueue);
    }

};
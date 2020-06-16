// DX12Engine.cpp
#include "pch.h"
#include "DX12Engine.h"
#include "SystemBase.h"
#include "d3dUtil.h"
#include "World.h"

static std::unique_ptr<World> world = nullptr;

int DX12Engine::Run(HINSTANCE hInstance)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        world = std::make_unique<World>();

        SystemBase system(hInstance);
        if (!system.Init())
            return 0;

        return system.Run();
    }
    catch (DxException & e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}
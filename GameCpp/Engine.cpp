#include "pch.h"
#include "Engine.h"
#include "../Common/Utility.h"

#pragma comment(lib, "runtimeobject.lib")

HWND g_hWnd = nullptr;
uint32_t g_DisplayWidth = 800;
uint32_t g_DisplayHeight = 600;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        //Graphics::Resize((UINT)(UINT64)lParam & 0xFFFF, (UINT)(UINT64)lParam >> 16);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

Engine::Engine(HINSTANCE hInstance)
{
}

Engine::~Engine()
{
}

void Engine::Run(const wchar_t* className)
{
    //ASSERT_SUCCEEDED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED));
    Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
    ASSERT_SUCCEEDED(InitializeWinRT);

    HINSTANCE hInst = GetModuleHandle(0);

    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = className;
    wcex.hIconSm = LoadIcon(hInst, IDI_APPLICATION);
    ASSERT(0 != RegisterClassEx(&wcex), "Unable to register a window");

    // Create window
    RECT rc = { 0, 0, (LONG)g_DisplayWidth, (LONG)g_DisplayHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd = CreateWindow(className, className, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInst, nullptr);

    ASSERT(g_hWnd != 0);

    //InitializeApplication(app);

    ShowWindow(g_hWnd, SW_SHOWDEFAULT);

    MSG msg = { 0 };

    while (msg.message != WM_QUIT)
    {
        // If there are Window messages then process them.
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Otherwise, do animation/game stuff.
        else
        {
            if (!m_AppPaused)
            {
                /*UpdateApplication(app)*/
                Sleep(100);
            }
            else
            {
                Sleep(100);
            }
        }
    }

    //Graphics::Terminate();
    //TerminateApplication(app);
    //Graphics::Shutdown();
}

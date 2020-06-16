// DXGame.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "DXGame.h"
#include "../DX12Engine/DX12Engine.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    return DX12Engine::Run(hInstance);
}

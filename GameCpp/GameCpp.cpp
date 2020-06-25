// GameCpp.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include "GameCpp.h"
#include "Engine.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    Engine* app = new Engine(hInstance);
    app->Run(L"GameCpp");
    delete app; 
    return 0;
}

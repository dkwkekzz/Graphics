#pragma once
#include "GameTimer.h"
#include "World.h"

class Draw
{
public:
	static void OnResize(World& world, int clientWidth, int clientHeight);
	static void OnChange4xMsaaState(World& world, int clientWidth, int clientHeight);
	static void OnRender(World& world, const GameTimer& timer);
};


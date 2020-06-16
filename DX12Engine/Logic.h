#pragma once
#include "GameTimer.h"
#include "World.h"

class Logic
{
public:
	static void OnInit(World& world, int clientWidth, int clientHeight);
	static void OnUpdate(World& world, const GameTimer& timer);
	static void OnDestroy(World& world);
};


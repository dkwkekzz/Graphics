#pragma once
#include "../DX12Engine/DX12Engine.h"


namespace P1G
{
	struct Update
	{
		static void OnUpdate()
		{
			//float t[3]{ 0, 0, 0 };
			//if (GetAsyncKeyState('A') & 0x8000)
			//	DX12Engine::OnDraw();
			//
			//if (GetAsyncKeyState('D') & 0x8000)
			//	t[0] += 1.1f;
			//
			//if (GetAsyncKeyState('W') & 0x8000)
			//	t[1] += 1.1f;
			//
			//if (GetAsyncKeyState('S') & 0x8000)
			//	t[1] -= 1.1f;
			//
			//// Don't let user move below ground plane.
			//t[1] = max(t[1], 0.0f);
			//
			//auto curve = cos(DX12Engine::GetTotalTime());
			//float s[3]{ curve, curve, curve };
			//float r[3]{ 0.0f, curve * 180.0f, 0.0f };
			//
			//DX12Engine::OnTransformActor(1, t, nullptr, nullptr);
		}
	};
}
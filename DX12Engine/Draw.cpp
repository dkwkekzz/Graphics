#include "pch.h"
#include "Draw.h"
#include "World.h"
#include "GlobalVar.h"

void Draw::OnResize(World& world, int clientWidth, int clientHeight)
{
	auto& d3d = world.GetD3D();
	if (!d3d.GetDevice())
		return;

	auto& heap = world.GetDescriptorHeap();
	d3d.Resize(heap, clientWidth, clientHeight);

	auto viewport = d3d.GetViewport();
	auto& cam = world.GetCamera();
	cam.SetOrtho(viewport.Width, viewport.Height, Global::SCREEN_NEAR, Global::SCREEN_DEPTH);
}

void Draw::OnChange4xMsaaState(World& world, int clientWidth, int clientHeight)
{
	auto& d3d = world.GetD3D();
	auto& heap = world.GetDescriptorHeap();
	d3d.Set4xMsaaState(!d3d.Get4xMsaaState());
	d3d.Resize(heap, clientWidth, clientHeight);
}

void Draw::OnRender(World& world, const GameTimer& timer)
{
}

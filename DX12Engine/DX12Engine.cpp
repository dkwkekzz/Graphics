// DX12Engine.cpp
#include "pch.h"
#include "DX12Engine.h"
#include "World.h"
#include "SLP.h"

static std::unique_ptr<World> world = nullptr;
static World* cachedWorld = nullptr;

#define WR &cachedWorld

void DX12Engine::OnInit()
{
	world = std::make_unique<World>();
	cachedWorld = world.get();

	SLP::GConstruct::CreateDevice(WR->GL, WR->CommandObject);
	SLP::GConstruct::CreateCommandObjects(WR->GL, WR->CommandObject);
	SLP::GConstruct::CreateSwapChain(WR->GL, WR->CommandObject);
	SLP::GConstruct::CreateDescriptorHeaps(WR->GL, WR->DescriptorHeap);
	SLP::GConstruct::InitializeAdvanced(WR->GL, WR->CommandObject, WR->DescriptorHeap, 
		WR->RootSignature, WR->TextureManager, WR->ShaderManager, WR->GeometryManager, 
		WR->MaterialManager, WR->RenderItemManager, WR->ResourceManager, WR->PSOManager, 
		WR->Camera);

	SLP::GCommander::Execute(WR->CommandObject);
	SLP::GCommander::Flush(WR->CommandObject);

}

void DX12Engine::OnDispose()
{
	if (!SLP::ValidDevice(WR->GL))
		return;

	SLP::GCommander::Flush(WR->CommandObject);
}

void DX12Engine::OnTick(bool appPaused)
{
	SLP::GUpdate::UpdateTimer(WR->GameTimer);
	SLP::GUpdate::UpdateFrame(WR->ResourceManager, WR->CommandObject);
	SLP::GUpdate::UpdateObjectCBs(WR->ResourceManager, WR->RenderItemManager);
	SLP::GUpdate::UpdateMaterialCBs(WR->ResourceManager, WR->MaterialManager);
	SLP::GUpdate::UpdateMainPassCB(WR->Camera, WR->PassBuffer, WR->GL, WR->GameTimer, WR->ResourceManager);
	SLP::GUpdate::UpdateReflectedPassCB(WR->PassBuffer, WR->ResourceManager);

	SLP::GRender::Draw(WR->ResourceManager, WR->CommandObject, WR->PSOManager, WR->GL, 
		WR->DescriptorHeap, WR->PassBuffer, WR->RootSignature, WR->RenderItemManager);
}

void DX12Engine::OnResize()
{
	SLP::GRender::Resize(WR->GL, WR->CommandObject, WR->DescriptorHeap, WR->Camera);
}

void DX12Engine::OnMouseDown(int btnState, int x, int y)
{
	SLP::GInput::OnMouseDown(WR->Mouse, x, y, WR->GL);
}

void DX12Engine::OnMouseUp(int btnState, int x, int y)
{
	SLP::GInput::OnMouseUp(x, y);
}

void DX12Engine::OnMouseMove(int btnState, int x, int y)
{
	SLP::GInput::OnMouseMove(btnState, x, y, WR->Mouse, WR->Camera);
}

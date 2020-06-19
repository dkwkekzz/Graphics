// DX12Engine.cpp
#include "pch.h"
#include "DX12Engine.h"
#include "World.h"
#include "SLP.h"

static std::unique_ptr<World> world = nullptr;
static World* cachedWorld = nullptr;

#define WR &cachedWorld

void DX12Engine::OnInit(HWND mainWnd, int width, int height)
{
	world = std::make_unique<World>();
	cachedWorld = world.get();

	SLP::GConstruct::CreateHandle(WR->GL, mainWnd, width, height);
	SLP::GConstruct::CreateDevice(WR->GL, WR->CommandObject);
	SLP::GConstruct::CreateCommandObjects(WR->GL, WR->CommandObject);
	SLP::GConstruct::CreateSwapChain(WR->GL, WR->CommandObject);
	SLP::GConstruct::CreateDescriptorHeaps(WR->GL, WR->DescriptorHeap);

	// Reset the command list to prep for initialization commands.
	SLP::GCommander::ResetDirectly(WR->CommandObject);

	// for test build
	SLP::GForTest::BuildTextures(WR->TextureManager);
	SLP::GForTest::BuildShaders(WR->ShaderManager);
	SLP::GForTest::BuildMaterials(WR->TextureManager, WR->MaterialManager);
	SLP::GForTest::BuildRenderItems(WR->MaterialManager, WR->GeometryManager, WR->RenderItemManager);

	SLP::GUpdate::UpdateTextures(WR->TextureManager, WR->GL, WR->CommandObject, WR->DescriptorHeap);
	SLP::GUpdate::UpdateGeometries(WR->GL, WR->CommandObject, WR->GeometryManager);
	SLP::GConstruct::BuildRootSignature(WR->GL, WR->RootSignature);
	SLP::GConstruct::BuildFrameResources(WR->GL, WR->ResourceManager);
	SLP::GConstruct::BuildPSOs(WR->GL, WR->PSOManager, WR->RootSignature, WR->ShaderManager);

	SLP::GCommander::Execute(WR->CommandObject);
	SLP::GCommander::Flush(WR->CommandObject);

	SLP::GConstruct::InitializeUtils(WR->Camera);
}

void DX12Engine::OnDispose()
{
	if (cachedWorld == nullptr)
		return;

	if (!SLP::ValidDevice(WR->GL))
		return;

	SLP::GCommander::Flush(WR->CommandObject);
}

void DX12Engine::OnTick(bool appPaused)
{
	SLP::GUpdate::UpdateTimer(WR->GameTimer);
	SLP::GInput::OnKeyboardInput(WR->GameTimer, WR->Camera);
	SLP::GUpdate::UpdateTextures(WR->TextureManager, WR->GL, WR->CommandObject, WR->DescriptorHeap);
	SLP::GUpdate::UpdateGeometries(WR->GL, WR->CommandObject, WR->GeometryManager);
	SLP::GUpdate::UpdateFrame(WR->ResourceManager, WR->CommandObject);
	SLP::GUpdate::UpdateObjectCBs(WR->ResourceManager, WR->RenderItemManager);
	SLP::GUpdate::UpdateMaterialCBs(WR->ResourceManager, WR->MaterialManager);
	SLP::GUpdate::UpdateMainPassCB(WR->Camera, WR->ResourceManager, WR->GL, WR->GameTimer);
	SLP::GUpdate::UpdateReflectedPassCB(WR->ResourceManager);
}

void DX12Engine::OnDraw()
{
	SLP::GCommander::ResetByCurrFrame(WR->CommandObject, WR->ResourceManager);

	SLP::GRender::Begin(WR->GL, WR->CommandObject);

	SLP::GRender::SetRenderTarget(WR->DescriptorHeap, WR->GL, WR->CommandObject, WR->ResourceManager);
	SLP::GRender::SetDescriptorHeaps(WR->DescriptorHeap, WR->CommandObject);
	SLP::GRender::SetRootSignature(WR->CommandObject, WR->RootSignature);

	// Draw opaque items--floors, walls, skull.
	SLP::GRender::SetPassConstantsBuffer(WR->ResourceManager, WR->CommandObject, 1);
	SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Opaque);

	// Mark the visible mirror pixels in the stencil buffer with the value 1
	SLP::GRender::SetStencil(WR->CommandObject, 1);
	SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Mirrors);

	// Draw the reflection into the mirror only (only for pixels where the stencil buffer is 1).
	// Note that we must supply a different per-pass constant buffer--one with the lights reflected.
	SLP::GRender::SetPassConstantsBuffer(WR->ResourceManager, WR->CommandObject, 2);
	SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Reflected);

	// Restore main pass constants and stencil ref.
	SLP::GRender::SetPassConstantsBuffer(WR->ResourceManager, WR->CommandObject, 1);
	SLP::GRender::SetStencil(WR->CommandObject, 0);

	// Draw mirror with transparency so reflection blends through.
	SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Transparent);

	// Draw shadows
	SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Shadow);

	SLP::GRender::End(WR->GL, WR->CommandObject);

	SLP::GCommander::Execute(WR->CommandObject);

	SLP::GRender::SwapChainBuffer(WR->GL);

	SLP::GCommander::Signal(WR->CommandObject);
}

void DX12Engine::OnResize(int width, int height)
{
	SLP::GRender::SetWindow(WR->GL, width, height);

	// Flush before changing any resources.
	SLP::GCommander::Flush(WR->CommandObject);
	SLP::GCommander::ResetDirectly(WR->CommandObject);

	SLP::GRender::ResetChainBuffer(WR->GL, WR->DescriptorHeap);
	SLP::GRender::ResetDepthStencilView(WR->GL, WR->DescriptorHeap, WR->CommandObject);

	// Execute the resize commands.
	SLP::GCommander::Execute(WR->CommandObject);

	// Wait until resize is complete.
	SLP::GCommander::Flush(WR->CommandObject);

	SLP::GRender::ResetViewport(WR->GL, WR->Camera);
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

void DX12Engine::OnCreateTexture(LPCSTR name, LPCWSTR filename)
{
}

void DX12Engine::OnCreateModel(LPCSTR name, LPCWSTR filename)
{
}

void DX12Engine::OnCreateShader(LPCSTR name, LPCWSTR filename)
{
}

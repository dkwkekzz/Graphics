// DX12Engine.cpp
#include "pch.h"
#include "DX12Engine.h"
#include "World.h"
#include "SLP.h"

static World* world = nullptr;

#define WR &world

void DX12Engine::OnInit(HWND mainWnd, int width, int height)
{
	world = new World;

	SLP::GConstruct::CreateHandle(WR->GL, mainWnd, width, height);
	SLP::GConstruct::CreateDevice(WR->GL, WR->CommandObject);
	SLP::GConstruct::CreateCommandObjects(WR->GL, WR->CommandObject);
	SLP::GConstruct::CreateSwapChain(WR->GL, WR->CommandObject);
	SLP::GConstruct::CreateDescriptorHeaps(WR->GL, WR->DescriptorHeap);
	SLP::GConstruct::CreateBlur(WR->BlurFilter, WR->GL, WR->DescriptorHeap);

	DX12Engine::OnResize(width, height);

	// Reset the command list to prep for initialization commands.
	SLP::GCommander::ResetDirectly(WR->CommandObject);

	SLP::GBuild::BuildMeshes(WR->GeometryManager);
	SLP::GPostProcess::ProcessMeshes(WR->GL, WR->CommandObject, WR->GeometryManager);
	SLP2G::GMesh::BuildWavesGeometry(WR->GL, WR->CommandObject, WR->GeometryManager, WR->Waves);
	SLP2G::GMesh::BuildRoomGeometry(WR->GL, WR->CommandObject, WR->GeometryManager);
	SLP2G::GMesh::BuildLandGeometry(WR->GL, WR->CommandObject, WR->GeometryManager);
	SLP2G::GMesh::BuildBoxGeometry(WR->GL, WR->CommandObject, WR->GeometryManager);
	SLP2G::GMesh::BuildTreeSpritesGeometry(WR->GL, WR->CommandObject, WR->GeometryManager);

	SLP::GBuild::BuildTextures(WR->TextureManager);
	SLP::GPostProcess::ProcessTextures(WR->TextureManager, WR->GL, WR->CommandObject, WR->DescriptorHeap);
	SLP::GBuild::BuildShaders(WR->ShaderManager);
	SLP::GBuild::BuildMaterials(WR->TextureManager, WR->MaterialManager);
	SLP::GBuild::BuildActors(WR->ActorCollection, WR->GeometryManager, WR->MaterialManager, WR->RenderItemManager);
	SLP::GBuild::BuildRootSignature(WR->GL, WR->RootSignature);
	SLP::GBuild::BuildFrameResources(WR->GL, WR->ResourceManager, WR->Waves);
	SLP::GBuild::BuildInputLayer(WR->PSOManager);
	SLP::GBuild::BuildPSOs(WR->GL, WR->PSOManager, WR->RootSignature, WR->ShaderManager);

	SLP::GCommander::Execute(WR->CommandObject);
	SLP::GCommander::Flush(WR->CommandObject);

	SLP::GConstruct::CreateEtcs(WR->GameTimer, WR->Camera);
}

void DX12Engine::OnDispose()
{
	if (world == nullptr)
		return;

	if (!ValidDevice(WR->GL))
		return;

	SLP::GCommander::Flush(WR->CommandObject);

	delete world;
}

void DX12Engine::OnTick(bool appPaused)
{
	SLP::GUpdate::UpdateTimer(WR->GameTimer);
	SLP::GInput::OnKeyboardInput(WR->GameTimer, WR->Camera);
	SLP::GPostProcess::ProcessMeshes(WR->GL, WR->CommandObject, WR->GeometryManager);
	SLP::GPostProcess::ProcessTextures(WR->TextureManager, WR->GL, WR->CommandObject, WR->DescriptorHeap);
	SLP::GUpdate::UpdateFrame(WR->ResourceManager, WR->CommandObject);
	SLP::GAnimate::AnimateMaterials(WR->MaterialManager, WR->GameTimer);
	SLP::GUpdate::UpdateObjectCBs(WR->ResourceManager, WR->ActorCollection);
	SLP::GUpdate::UpdateMaterialCBs(WR->ResourceManager, WR->MaterialManager);
	SLP::GUpdate::UpdateMainPassCB(WR->Camera, WR->ResourceManager, WR->GL, WR->GameTimer);
	SLP::GUpdate::UpdateReflectedPassCB(WR->ResourceManager);
	SLP::GUpdate::UpdateWaves(WR->GameTimer, WR->Waves, WR->ResourceManager, WR->RenderItemManager);
}

// TODO: 하나의 함수로 통합한다. 액터는 따로 관리하지 않고 하위레이어로 내린다. 그리고 드로우인자만 받아 저장하여 그대로 출력한다.
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

	SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::AlphaTested);

	SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::AlphaTestedTreeSprites);

	SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Transparent);

	if ((WR->GraphicOption)->useBlur)
	{
		SLP::GRender::PostProcess(WR->BlurFilter, WR->CommandObject, WR->RootSignature, WR->PSOManager, WR->GL);
	}
	else
	{
		SLP::GRender::End(WR->GL, WR->CommandObject);
	}

	//// Mark the visible mirror pixels in the stencil buffer with the value 1
	//SLP::GRender::SetStencil(WR->CommandObject, 1);
	//SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Mirrors);
	//
	//// Draw the reflection into the mirror only (only for pixels where the stencil buffer is 1).
	//// Note that we must supply a different per-pass constant buffer--one with the lights reflected.
	//SLP::GRender::SetPassConstantsBuffer(WR->ResourceManager, WR->CommandObject, 2);
	//SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Reflected);
	//
	//// Restore main pass constants and stencil ref.
	//SLP::GRender::SetPassConstantsBuffer(WR->ResourceManager, WR->CommandObject, 1);
	//SLP::GRender::SetStencil(WR->CommandObject, 0);
	//
	//// Draw mirror with transparency so reflection blends through.
	//SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Transparent);
	//
	//// Draw shadows
	//SLP::GRender::Draw(WR->CommandObject, WR->PSOManager, WR->ResourceManager, WR->RenderItemManager, WR->DescriptorHeap, SLO::RenderLayer::Shadow);

	SLP::GCommander::Execute(WR->CommandObject);

	SLP::GRender::SwapChainBuffer(WR->GL);

	SLP::GCommander::SignalByCurrFrame(WR->CommandObject, WR->ResourceManager);
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

	SLP::GRender::ResetBlur(WR->BlurFilter, width, height);
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

void DX12Engine::OnRestart()
{
	if (world == nullptr)
		return;

	SLP::GTimer::Restart(WR->GameTimer);
}

void DX12Engine::OnPause()
{
	if (world == nullptr)
		return;

	SLP::GTimer::Pause(WR->GameTimer);
}

float DX12Engine::GetTotalTime()
{
	if (world == nullptr)
		return 0.0f;

	return GameTotalTime(WR->GameTimer);
}

void DX12Engine::OnCreateActor(LPCSTR modelName, int& generatedId)
{
	SLP2G::GActor::CreateActor(WR->ActorCollection, (WR->GeometryManager)->geometries[modelName].get(), generatedId);
}

void DX12Engine::OnTransformActor(int id, float translate[3], float rotate[3], float scale[3])
{
	SLP2G::GActor::TransformActor(WR->ActorCollection, id, CalcMatrix(translate, rotate, scale));
}

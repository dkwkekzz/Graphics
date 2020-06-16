#include "pch.h"
#include "Logic.h"

void Logic::OnInit(World& world, int clientWidth, int clientHeight)
{
    auto& d3d = world.GetD3D();
	d3d.Init();

    auto& heap = world.GetDescriptorHeap();
    auto* device = d3d.GetDevice();
    heap.Init(device);
    d3d.Resize(heap, clientWidth, clientHeight);

    auto& cam = world.GetCamera();
    cam.SetPosition(0.0f, 2.0f, -15.0f);

    auto& texMap = world.GetTextureMap();
    auto* cmdList = d3d.GetCommandList();
    texMap.Add(device, cmdList, heap, "grassTex", L"Textures/grass.dds");
    texMap.Add(device, cmdList, heap, "waterTex", L"Textures/water1.dds");
    texMap.Add(device, cmdList, heap, "fenceTex", L"Textures/WoodCrate01.dds");

    //BuildDescriptorHeaps();
    //BuildRootSignature();
    //BuildShadersAndInputLayout();
    //BuildLandGeometry();
    //BuildBoxGeometry();
    //BuildQuadGeometry();
    //BuildMaterials();
    //BuildRenderItems();
    //BuildFrameResources();
    //BuildPSOs();

}

void Logic::OnUpdate(World& world, const GameTimer& timer)
{
}

void Logic::OnDestroy(World& world)
{
	auto& d3d = world.GetD3D();
	if (d3d.GetDevice() != nullptr)
		d3d.FlushCommandQueue();
}

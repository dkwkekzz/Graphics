#include "pch.h"
#include "RenderLayers.h"
#include "GL.h"
#include "PSOMap.h"

Opaque = 0,
Mirrors,
Reflected,
Transparent,
Shadow,

void RenderLayers::Init(GL* gl, PSOMap* psos)
{
	auto opaqueBundle = std::make_unique<RenderBundle>();
	opaqueBundle->pipelineState = psos->Get("opaque");
	opaqueBundle->passCBIndex = 0;
	opaqueBundle->useStencil = false;
	opaqueBundle->stencilRef = 0;
	mRitemLayer[(int)RenderLayer::Opaque] = std::move(opaqueBundle);

	auto mirrorBundle = std::make_unique<RenderBundle>();
	mirrorBundle->pipelineState = psos->Get("opaque");
	mirrorBundle->passCBIndex = 0;
	mirrorBundle->useStencil = false;
	mirrorBundle->stencilRef = 0;
	mRitemLayer[(int)RenderLayer::Mirrors] = std::move(opaqueBundle);

	auto opaqueBundle = std::make_unique<RenderBundle>();
	opaqueBundle->pipelineState = psos->Get("opaque");
	opaqueBundle->passCBIndex = 0;
	opaqueBundle->useStencil = false;
	opaqueBundle->stencilRef = 0;
	mRitemLayer[(int)RenderLayer::Opaque] = std::move(opaqueBundle);

	auto opaqueBundle = std::make_unique<RenderBundle>();
	opaqueBundle->pipelineState = psos->Get("opaque");
	opaqueBundle->passCBIndex = 0;
	opaqueBundle->useStencil = false;
	opaqueBundle->stencilRef = 0;
	mRitemLayer[(int)RenderLayer::Opaque] = std::move(opaqueBundle);

	auto opaqueBundle = std::make_unique<RenderBundle>();
	opaqueBundle->pipelineState = psos->Get("opaque");
	opaqueBundle->passCBIndex = 0;
	opaqueBundle->useStencil = false;
	opaqueBundle->stencilRef = 0;
	mRitemLayer[(int)RenderLayer::Opaque] = std::move(opaqueBundle);
}

void RenderLayers::AddItem(RenderLayer layer, Matrix matrix, Material* material, MeshGeometry* mesh, int submeshIdx)
{
	auto floorRitem = std::make_unique<RenderItem>();
	floorRitem->World = matrix;
	floorRitem->TexTransform = MathHelper::Identity4x4();
	floorRitem->ObjCBIndex = mAllRitems.size();
	floorRitem->Mat = material;
	floorRitem->Geo = mesh;
	floorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	floorRitem->IndexCount = floorRitem->Geo->SubMeshes[submeshIdx].IndexCount;
	floorRitem->StartIndexLocation = floorRitem->Geo->SubMeshes[submeshIdx].StartIndexLocation;
	floorRitem->BaseVertexLocation = floorRitem->Geo->SubMeshes[submeshIdx].BaseVertexLocation;

	auto* bundle = mRitemLayer[(int)layer].get();
	bundle->ritems.push_back(floorRitem.get());

	mAllRitems.push_back(std::move(floorRitem));
}
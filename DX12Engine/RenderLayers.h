#pragma once
#include "TypeDefine.h"
#include "RenderItem.h"

class GL;

enum class RenderLayer : int
{
	Opaque = 0,
	Mirrors,
	Reflected,
	Transparent,
	Shadow,
	Count
};

struct RenderBundle
{
	TSignature* rootSignature;
	TPSO* pipelineState;
	int passCBIndex;
	std::vector<RenderItem*> ritems;
	bool useStencil;
	int stencilRef;
};

class RenderLayers
{
public:
	void Init(GL* gl);
	void AddItem();

private:
	void BuildRootSignature(GL* gl);

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_defaultRootSignature;
	std::unique_ptr<RenderBundle> mRitemLayer[(int)RenderLayer::Count];
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

};


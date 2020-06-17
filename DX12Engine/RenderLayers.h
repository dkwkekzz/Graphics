#pragma once
#include "TypeDefine.h"
#include "RenderItem.h"

class GL;
class PSOMap;

struct RenderBundle
{
	TPSO* pipelineState;
	int passCBIndex;
	std::vector<RenderItem*> ritems;
	bool useStencil;
	int stencilRef;
};

class RenderLayers
{
public:
	inline const RenderBundle* GetBundle(RenderLayer layer) { return mRitemLayer[(int)layer].get(); }
	void Init(GL* gl, PSOMap* psos);
	void AddItem(RenderLayer layer, Matrix matrix, Material* material, MeshGeometry* mesh, int submeshIdx = 0);

private:
	std::unique_ptr<RenderBundle> mRitemLayer[(int)RenderLayer::Count];
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

};


#pragma once
#include "TypeDefine.h"

class PSOMap
{
public:
	inline TPSO* Get(const std::string& name) { return mPSOs[name]; }
	void Init();

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

};


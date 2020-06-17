#pragma once
#include "TypeDefine.h"
#include "d3dx12.h"

class GL;
class ShaderMap;

class PSOMap
{
public:
	inline TPSO* Get(const std::string& name) const { return mPSOs.find(name)->second.Get(); }
	void Init(const GL* gl, const ShaderMap* shaders);

private:
	void BuildRootSignature(int numParams, CD3DX12_ROOT_PARAMETER* slotRootParameter, TDevice* device, Microsoft::WRL::ComPtr<ID3D12RootSignature>& outRootSignature);

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

};


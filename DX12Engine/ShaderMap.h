#pragma once
#include "TypeDefine.h"

class ShaderMap
{
public:
	inline TShader* Get(const std::string& name) const { return m_shaders.find(name)->second.Get(); }
	void Init();

private:
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_shaders;

};


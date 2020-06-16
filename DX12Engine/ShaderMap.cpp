#include "pch.h"
#include "ShaderMap.h"
#include "d3dUtil.h"

void ShaderMap::Init()
{
	m_shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
	m_shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");
}

#include "pch.h"
#include "MaterialMap.h"

using namespace DirectX;

void MaterialMap::Add(const std::string& name)
{
    auto idx = m_materials.size();
    auto mat = std::make_unique<Material>();
    mat->Name = name;
    mat->MatCBIndex = idx;
    mat->DiffuseSrvHeapIndex = idx;
    mat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    mat->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    mat->Roughness = 0.125f;

    m_materials.emplace(name, mat);
}

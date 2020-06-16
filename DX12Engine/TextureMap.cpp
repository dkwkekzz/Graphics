#include "pch.h"
#include "TextureMap.h"
#include "DDSTextureLoader.h"

void TextureMap::Add(ID3D12Device* d3dDevice, ID3D12GraphicsCommandList* cmdList, const DescriptorHeap& heap, const std::string& name, const std::wstring& filename)
{
    auto texIdx = mTextures.size();
    auto tex = std::make_unique<Texture>();
    tex->Name = name;
    tex->Filename = filename;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice, cmdList, 
        tex->Filename.c_str(), tex->Resource, tex->UploadHeap));

    auto res = tex->Resource;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = res->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor = heap.SrvHeapHandle();
    hDescriptor.Offset(texIdx, heap.SrvDescriptorSize());

    d3dDevice->CreateShaderResourceView(res.Get(), &srvDesc, hDescriptor);

    mTextures[tex->Name] = std::move(tex);
}

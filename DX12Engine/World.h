#pragma once
#include "GameTimer.h"
#include "Camera.h"
#include "SLO.h"

#define getter(cls)												\
    private: std::unique_ptr<cls> m##cls;					    \
    public: inline cls* Get##cls() const { m##cls.get(); }		\

struct World
{
    GameTimer GameTimer; 
    Camera Camera;
    SLO::CommandObject CommandObject;
    SLO::DescriptorHeap DescriptorHeap;
    SLO::ResourceManager ResourceManager;
    SLO::GeometryManager GeometryManager;
    SLO::GL GL;
    SLO::MaterialManager MaterialManager;
    SLO::Mouse Mouse;
    SLO::PSOManager PSOManager;
    SLO::RenderItemManager RenderItemManager;
    SLO::RootSignature RootSignature;
    SLO::ShaderManager ShaderManager;
    SLO::TextureManager TextureManager;
};
#pragma once
#include "GameTimer.h"
#include "Camera.h"
#include "Waves.h"
#include "BlurFilter.h"
#include "SLO.h"

#define getter(cls)												\
    private: std::unique_ptr<cls> m##cls;					    \
    public: inline cls* Get##cls() const { m##cls.get(); }		\

struct World
{
    GameTimer GameTimer; 
    Camera Camera;
    Waves Waves{ 128, 128, 1.0f, 0.03f, 4.0f, 0.2f };
    BlurFilter BlurFilter;
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
    SLO::ActorCollection ActorCollection;
    SLO::GraphicOption GraphicOption;
};
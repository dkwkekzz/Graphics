#pragma once
#include "GameTimer.h"
#include "GL.h"
#include "Camera.h"
#include "DescriptorHeap.h"
#include "TextureMap.h"
#include "MaterialMap.h"
#include "GeometryMap.h"
#include "ShaderMap.h"
#include "PSOMap.h"
#include "CommandObject.h"
#include "RenderLayers.h"
#include "FrameManager.h"

#define getter(cls)												\
    private: std::unique_ptr<cls> m##cls;					    \
    public: inline cls* Get##cls() const { m##cls.get(); }		\

class World
{
    getter(GameTimer)
    getter(GL)
    getter(Camera)
    getter(DescriptorHeap)
    getter(TextureMap)
    getter(MaterialMap)
    getter(GeometryMap)
    getter(ShaderMap)
    getter(PSOMap)
    getter(CommandObject)
    getter(RenderLayers)
    getter(FrameManager)

public:
	World();

};
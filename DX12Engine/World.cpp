#include "pch.h"
#include "World.h"
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

using namespace std;

World::World() : 
	mGameTimer(make_unique<GameTimer>())
,	mGL(make_unique<GL>())
,	mCamera(make_unique<Camera>())
,	mDescriptorHeap(make_unique<DescriptorHeap>())
,	mTextureMap(make_unique<TextureMap>())
,	mMaterialMap(make_unique<MaterialMap>())
,	mGeometryMap(make_unique<GeometryMap>())
,	mShaderMap(make_unique<ShaderMap>())
,	mPSOMap(make_unique<PSOMap>())
,	mCommandObject(make_unique<CommandObject>())
,	mRenderLayers(make_unique<RenderLayers>())
{
}
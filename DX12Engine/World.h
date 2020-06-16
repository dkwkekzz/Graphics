#pragma once

class GameTimer;
class GL;
class Camera;
class DescriptorHeap;
class TextureMap;
class MaterialMap;
class GeometryMap;
class ShaderMap;
class PSOMap;
class CommandObject;
class RenderLayers;

class World
{
public:
	inline GameTimer* GetGameTimer() const { mGameTimer.get(); }
	inline GL* GetGL() const { mGL.get(); }
	inline Camera* GetCamera() const { mCamera.get(); }
	inline DescriptorHeap* GetDescriptorHeap() const { mDescriptorHeap.get(); }
	inline TextureMap* GetTextureMap() const { mTextureMap.get(); }
	inline MaterialMap* GetMaterialMap() const { mMaterialMap.get(); }
	inline GeometryMap* GetGeometryMap() const { mGeometryMap.get(); }
	inline ShaderMap* GetShaderMap() const { mShaderMap.get(); }
	inline PSOMap* GetPSOMap() const { mPSOMap.get(); }
	inline CommandObject* GetCommandObject() const { mCommandObject.get(); }
	inline RenderLayers* GetRenderLayers() const { mRenderLayers.get(); }

	World();

private:
	std::unique_ptr<GameTimer>		mGameTimer;
	std::unique_ptr<GL>				mGL;
	std::unique_ptr<Camera>			mCamera;
	std::unique_ptr<DescriptorHeap>	mDescriptorHeap;
	std::unique_ptr<TextureMap>		mTextureMap;
	std::unique_ptr<MaterialMap>	mMaterialMap;
	std::unique_ptr<GeometryMap>	mGeometryMap;
	std::unique_ptr<ShaderMap>		mShaderMap;
	std::unique_ptr<PSOMap>			mPSOMap;
	std::unique_ptr<CommandObject>	mCommandObject;
	std::unique_ptr<RenderLayers>	mRenderLayers;

};
#pragma once
#include "pch.h"
#include "GlobalVar.h"
#include "d3dx12.h"
#include "d3dUtil.h"
#include "UploadBuffer.h"

#include "Camera.h"
#include "GameTimer.h"

namespace SLO
{
	struct GL
	{
		Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
		Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
		D3D_DRIVER_TYPE d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;

		//Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
		//Microsoft::WRL::ComPtr<ID3D12CommandAllocator> directCmdListAlloc;
		//Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
		//
		//Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		//UINT64 currentFence = 0;

		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
		Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffer[Global::SWAP_CHAIN_BUFFER_COUNT];
		int currBackBuffer = 0;
		DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;
		DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// Set true to use 4X MSAA (?.1.8).  The default is false.
		bool Msaa4xState = false;    // 4X MSAA enabled
		UINT Msaa4xQuality = 0;      // quality level of 4X MSAA

		D3D12_VIEWPORT screenViewport;
		D3D12_RECT scissorRect;

		//UINT rtvDescriptorSize = 0;
		//UINT dsvDescriptorSize = 0;
		//UINT cbvSrvUavDescriptorSize = 0;

		HWND mainWnd;
		int clientWidth;
		int clientHeight;

		//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
		//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
		//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;

		//inline D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const
		//{
		//	return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currBackBuffer, rtvDescriptorSize);
		//}
		//
		//inline D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const
		//{
		//	return dsvHeap->GetCPUDescriptorHandleForHeapStart();
		//}
	};

	struct CommandObject
	{
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> directCmdListAlloc;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		UINT64 currentFence = 0;
	};

	struct DescriptorHeap
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap = nullptr;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap = nullptr;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;

		UINT cbvSrvUavDescriptorSize = 0;
		UINT rtvDescriptorSize = 0;
		UINT dsvDescriptorSize = 0;

		int srvCount = 0;
	};

	struct RootSignature
	{
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	};

	struct Texture
	{
		std::string name;
		std::wstring filename;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap = nullptr;

		int srvOffset;
	};

	struct TextureManager
	{
		std::queue<std::pair<std::string, std::wstring> > waitqueue;
		std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
	};

	struct ShaderManager
	{
		std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> shaders;
	};

	struct Vertex
	{
		Vertex() = default;
		Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) :
			Pos(x, y, z),
			Normal(nx, ny, nz),
			TexC(u, v) {}

		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 TexC;
	};

	struct SubmeshGeometry
	{
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		INT BaseVertexLocation = 0;

		DirectX::BoundingBox Bounds;
	};

	struct MeshGeometry
	{
		// Give it a name so we can look it up by name.
		std::string Name;

		// System memory copies.  Use Blobs because the vertex/index format can be generic.
		// It is up to the client to cast appropriately.  
		Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

		// Data about the buffers.
		UINT VertexByteStride = 0;
		UINT VertexBufferByteSize = 0;
		DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
		UINT IndexBufferByteSize = 0;

		// A MeshGeometry may store multiple geometries in one vertex/index buffer.
		// Use this container to define the Submesh geometries so we can draw
		// the Submeshes individually.
		std::vector<SubmeshGeometry> Submeshes;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
		{
			D3D12_VERTEX_BUFFER_VIEW vbv;
			vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
			vbv.StrideInBytes = VertexByteStride;
			vbv.SizeInBytes = VertexBufferByteSize;

			return vbv;
		}

		D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
		{
			D3D12_INDEX_BUFFER_VIEW ibv;
			ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
			ibv.Format = IndexFormat;
			ibv.SizeInBytes = IndexBufferByteSize;

			return ibv;
		}

		// We can free this memory after we finish upload to the GPU.
		void DisposeUploaders()
		{
			VertexBufferUploader = nullptr;
			IndexBufferUploader = nullptr;
		}
	};

	struct GeometryManager
	{
		std::queue<std::pair<std::string, std::wstring> > waitqueue;
		std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> geometries;
	};

	struct Light
	{
		DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
		float FalloffStart = 1.0f;                          // point/spot light only
		DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
		float FalloffEnd = 10.0f;                           // point/spot light only
		DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
		float SpotPower = 64.0f;                            // spot light only
	};

	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	};

	struct PassConstants
	{
		DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
		float cbPerObjectPad1 = 0.0f;
		DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
		DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
		float NearZ = 0.0f;
		float FarZ = 0.0f;
		float TotalTime = 0.0f;
		float DeltaTime = 0.0f;

		DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

		DirectX::XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		float gFogStart = 5.0f;
		float gFogRange = 150.0f;
		DirectX::XMFLOAT2 cbPerObjectPad2;

		// Indices [0, NUM_DIR_LIGHTS) are directional lights;
		// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
		// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
		// are spot lights for a maximum of MaxLights per object.
		Light Lights[MaxLights];
	};

	struct MaterialConstants
	{
		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = 0.25f;

		// Used in texture mapping.
		DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
	};

	// Simple struct to represent a material for our demos.  A production 3D engine
	// would likely create a class hierarchy of Materials.
	struct Material
	{
		// Unique material name for lookup.
		std::string Name;

		// Index into constant buffer corresponding to this material.
		int MatCBIndex = -1;

		// diffuse texture.
		Texture* DiffuseTex;

		// Dirty flag indicating the material has changed and we need to update the constant buffer.
		// Because we have a material constant buffer for each FrameResource, we have to apply the
		// update to each FrameResource.  Thus, when we modify a material we should set 
		// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
		int NumFramesDirty = Global::FRAME_RESOURCE_COUNT;

		// Material constant buffer data used for shading.
		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = .25f;
		DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
	};

	struct MaterialManager
	{
		std::unordered_map<std::string, std::unique_ptr<Material>> materials;
	};

	enum class RenderLayer : int
	{
		None = 0,
		Opaque,
		Mirrors,
		Reflected,
		Transparent,
		Shadow,
		Count
	};

	// Lightweight structure stores parameters to draw a shape.  This will
	// vary from app-to-app.
	struct RenderItem
	{
		RenderItem() = default;

		// World matrix of the shape that describes the object's local space
		// relative to the world space, which defines the position, orientation,
		// and scale of the object in the world.
		DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();

		DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

		// Dirty flag indicating the object data has changed and we need to update the constant buffer.
		// Because we have an object cbuffer for each FrameResource, we have to apply the
		// update to each FrameResource.  Thus, when we modify obect data we should set 
		// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
		int NumFramesDirty = Global::FRAME_RESOURCE_COUNT;

		// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
		UINT ObjCBIndex = -1;

		Material* Mat = nullptr;
		MeshGeometry* Geo = nullptr;

		// Primitive topology.
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		// DrawIndexedInstanced parameters.
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		int BaseVertexLocation = 0;
	};

	struct RenderBundle
	{
		std::vector<RenderItem*> items;
	};

	struct RenderItemManager
	{
		// List of all the render items.
		std::vector<std::unique_ptr<RenderItem>> allritems;
		int itemCount = 0;

		// Render items divided by PSO.
		RenderBundle ritemLayer[(int)RenderLayer::Count];
	};

	struct FrameResource
	{
		// We cannot reset the allocator until the GPU is done processing the commands.
		// So each frame needs their own allocator.
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

		// We cannot update a cbuffer until the GPU is done processing the commands
		// that reference it.  So each frame needs their own cbuffers.
	   // std::unique_ptr<UploadBuffer<FrameConstants>> FrameCB = nullptr;
		std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
		std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
		std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

		// Fence value to mark commands up to this fence point.  This lets us
		// check if these frame resources are still in use by the GPU.
		UINT64 Fence = 0;
	};

	struct ResourceManager
	{
		std::vector<std::unique_ptr<FrameResource>> frameResources;
		FrameResource* currFrameResource = nullptr;
		int currFrameResourceIndex = 0;

		PassConstants mainPassCB;
		PassConstants reflectedPassCB;
	};

	struct PSOManager
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> PSOs[(int)RenderLayer::Count];
	};

	struct Mouse
	{
		POINT lastMousePos;
	};
}
